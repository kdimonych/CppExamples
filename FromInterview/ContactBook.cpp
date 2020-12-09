/**
    It is needed to build contact book class. Each contact of the contact book has to have such fields: number, contact, email.
    There can be multiple contacts with similar fields values. Add, remove, select and update operations should have complexity O(1)
*/

#include <iostream>
#include <unordered_map>
#include <map>
#include <list>
#include <vector>
#include <string>
#include <algorithm>
#include <utility>
#include <functional>
#include <iterator>

using namespace std;

class ContactBook
{
public:
    struct Contact {
        string name;
        string number;
        string email;
    };
    
    using const_iterator = list<Contact>::const_iterator;
    enum IndexField
    {
        Number,
        Email,
        Name,
        
        IdexFieldCount //Don't touch
    };
    
    template <typename NewContactT>
    void add(NewContactT&& newContact);
    template <typename ...Args>
    const_iterator emplace(Args&& ...args);
    bool remove(const const_iterator& contactIt);
    template <typename NewContactT>
    inline bool update(const const_iterator& contactIt, NewContactT&& newContact);
    
    inline vector<const_iterator> select(IndexField indexField, const string& key) const {
        assert(indexField < IdexFieldCount);
        return _selectFromIndexTable(m_indexers[indexField], key);
    }
    //Precise select by several field together
    vector<const_iterator> select(vector<pair<IndexField, string>> fields) const;
    
    inline size_t size() const noexcept { return m_contacts.size(); }
    
    const_iterator begin() const noexcept { return m_contacts.cbegin(); }
    const_iterator end() const noexcept { return m_contacts.cend(); }
    
private:
    using Key = string;
    using Contacts = list<Contact>;
    using IndexTable = unordered_multimap<Key, const_iterator>;
    
    static void _removeIndex(IndexTable& indexer, const string& key, const const_iterator& id);
    static vector<const_iterator> _selectFromIndexTable(const IndexTable& indexer, const string& key);
    static const string& _getFieldValue(const Contact& contact, IndexField field);
    
    IndexTable m_indexers[IdexFieldCount];
    Contacts m_contacts;
};

template <typename NewContactT>
void ContactBook::add(NewContactT&& newContact)
{
    auto it = m_contacts.insert(m_contacts.end(), forward<NewContactT>(newContact));
    
    //Update index tables
    m_indexers[Number].emplace(it->number, it);
    m_indexers[Email].emplace(it->email, it);
    m_indexers[Name].emplace(it->name, it);
}

template <typename ...Args>
ContactBook::const_iterator ContactBook::emplace(Args&& ...args)
{
    auto it = m_contacts.emplace(m_contacts.end(), forward<Args>(args)...);
    
    //Update index tables
    m_indexers[Number].emplace(it->number, it);
    m_indexers[Email].emplace(it->email, it);
    m_indexers[Name].emplace(it->name, it);
    
    return it;
}

bool ContactBook::remove(const const_iterator& contactIt)
{
    _removeIndex(m_indexers[Number], contactIt->number, contactIt);
    _removeIndex(m_indexers[Email], contactIt->email, contactIt);
    _removeIndex(m_indexers[Name], contactIt->name, contactIt);
    
    m_contacts.erase(contactIt);
}

template <typename NewContactT>
inline bool ContactBook::update(const const_iterator& contactIt, NewContactT&& newContact)
{
    assert(contactIt.operator->() != &newContact);
    remove(contactIt);
    add(forward(newContact));
    return false;
}

vector<ContactBook::const_iterator> ContactBook::select(vector<pair<IndexField, string>> fields) const {
    assert(fields.size() <= IdexFieldCount);
    vector<const_iterator> result;
    
    if(fields.size() > 0)
    {
        //Sort fields by number of equal keys in appropriate IndexTable (in descending order)
        sort(fields.begin(), fields.end(),
             [this](const map<IndexField, string>::value_type& a,
                    const map<IndexField, string>::value_type& b)
        {
            assert(a.first < IdexFieldCount);
            return m_indexers[a.first].count(a.second) < m_indexers[b.first].count(b.second);
        });
        
        // So here we have the fields sorted. The first element has the smallest amount of equal keys in appropriate IndexTable. Fill result with all of theese elements.
        auto fieldIt = fields.begin();
        result = select(fieldIt->first, fieldIt->second);
        
        for(++fieldIt ; fieldIt != fields.end(); ++fieldIt)
        {
            auto toRemove = remove_if(result.begin(), result.end(),
                      [fieldIt](const const_iterator& it){
                return _getFieldValue(*it, fieldIt->first) != fieldIt->second;
            });
            result.erase(toRemove, result.end());
        }
    }
    return result;
}

void ContactBook::_removeIndex(IndexTable& indexer, const string& key, const const_iterator& id)
{
    auto range = indexer.equal_range(key);
    for(;range.first != range.second; ++range.first)
    {
        if(range.first->second == id)
        {
            indexer.erase(range.first);
            break;
        }
    }
}

vector<ContactBook::const_iterator>
ContactBook::_selectFromIndexTable(const IndexTable& indexer, const string& key)
{
    vector<ContactBook::const_iterator> result;
    
    auto range = indexer.equal_range(key);
    result.reserve(indexer.count(key));
    for(;range.first != range.second; ++range.first)
        result.push_back(range.first->second);
    
    return result;
}

const string& ContactBook::_getFieldValue(const Contact& contact, IndexField field)
{
    assert(field < IdexFieldCount);
    static const string defaultVal;
    
    switch(field)
    {
        case Name:
            return contact.name;
        case Number:
            return contact.number;
        case Email:
            return contact.email;
        default:
            return defaultVal;
    }
}

static void print_contact(const ContactBook::Contact& contact)
{
    cout
    << "Name: " << contact.name << "; "
    << "Phone Number: " << contact.number << "; "
    << "Email: " << contact.email
    << endl;
}

int main(int argc, const char * argv[]) {
    ContactBook cb;
    
    cb.add(ContactBook::Contact{"Jane", "+44949034880", "jane@gmail.com"});
    cb.add(ContactBook::Contact{"Alex", "+44934589998", "alex@gmail.com"});
    cb.add(ContactBook::Contact{"Dan",  "+44000000001", "dan@gmail.com"});
    cb.add(ContactBook::Contact{"Dan",  "+44000000055", "dan@jandex.com"});
    
    cout << "Contact Book:" << endl;
    for(auto& contact : cb)
        print_contact(contact);
    cout << endl;
    
    cout << "Select contacts with name \"Dan\":" << endl;
    auto selected = cb.select(ContactBook::Name, "Dan");
    for(auto& itemIt : selected)
        print_contact(*itemIt);
    cout << endl;
    
    cout << "Select contacts with name \"Dan\" and number \"+44000000001\":" << endl;
    selected = cb.select({
        {ContactBook::Name, "Dan"},
        {ContactBook::Number, "+44000000001"}
    });
    for(auto& itemIt : selected)
        print_contact(*itemIt);
    cout << endl;
    
    cout << "Remove contacts with name \"Dan\" and number \"+44000000001\":" << endl;
    selected = cb.select({
        {ContactBook::Name, "Dan"},
        {ContactBook::Number, "+44000000001"}
    });
    for(auto& itemIt : selected)
        cb.remove(itemIt);
    cout << endl;
    
    cout << "Print Contact Book:" << endl;
    for(auto& contact : cb)
        print_contact(contact);
    cout << endl;
    
    return 0;
}
