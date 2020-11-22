#include <iostream>
#include <type_traits>

template<typename A, typename B>
constexpr auto ptrDiff(const A& a, const B& b)
{
    return reinterpret_cast<const char*>(&b) - reinterpret_cast<const char*>(&a);
}

/** This function calculates a relative shift to the data bloc of base class B
    relative to the object obj of derived class D. The class B must be subclass of class D.
 
 search tags:
    Calculates a relative shift to the data bloc of base class
 */

template<class B, class D>
constexpr auto baseClassShift(const D& d)
{
    static_assert(std::is_base_of<B, D>::value, "B is not base clas of D");
    return ptrDiff(d, static_cast<const B&>(d));
}

/** This function calculates a relative shift to the data member m of class C
    relative to the object obj of class O. The class C must be subclass of  class O.
 
 search tags:
    Passing of pointer to data member as a template argument
    Calculate a relative shift to the data member
 */
template<class O, typename C, typename M>
constexpr auto memberDataShift(const O& obj, const M C::* m)
{
    static_assert(std::is_base_of<C, O>::value, "C is not base of O");
    return ptrDiff(obj, obj.*m);
}

//This doesn't matter
//template<class O, typename C, typename Res, typename ...Args>
//constexpr auto memberDataPos(O& obj, Res (C::*m) (Args...))
//{
//    static_assert(std::is_base_of<C, O>::value, "C is not base of O");
//    return ptrDiff(obj, m);
//}

int main(int argc, const char * argv[]) {
    using namespace std;
    struct M{
        int m;
    };
    struct A/*: public M */{
        A(){cout << "A()" << endl;}
        int a;
        int func() {cout << "int A::func()" << endl; return 0;};
    };
    struct B/*: public M */{
        B(){cout << "B()" << endl;}
        int b;
    };
    struct C: public M{
        C(){cout << "C()" << endl;}
        int c;
    };
    struct D: virtual public A,  virtual public B/*, virtual public  B, virtual public C*/ {
        D() {cout << "D()" << endl;}
        int func() {cout << "int D::func()" << endl; return 0;};
        //virtual ~D() = default;
        int d;
    };
    
    D d;
    cout << "sizeof(d) = " << sizeof(d) << endl;
    cout << "D ptr = " << baseClassShift<D>(d) << endl;
    cout << "D.d ptr = " << memberDataShift(d, &D::d) << endl;
    cout << "A ptr = " << baseClassShift<A>(d) << endl;
    cout << "A.a ptr = " << memberDataShift(d, &A::a) << endl;
    cout << "B ptr = " << baseClassShift<B>(d) << endl;
    cout << "B.b ptr = " << memberDataShift(d, &B::b) << endl;
}
