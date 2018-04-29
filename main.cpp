

#include<string>
#include<iostream>

#include <map>
#include "charset.h"
#include "prefix_tree.h"

int main()
{
    struct toto
    {
        int a;

        toto()
        :a(0)
        {

        }

        explicit toto(int a)
        :a(a)
        {
        }

        explicit toto(const toto & t)
        :a(t.a)
        {

        }

    };
    typedef prefix_tree<std::string, toto, ascii_charset> tree;
    tree t = tree(ascii_charset());

    std::cout << "empty " << t.empty() << std::endl;

    toto to;
    t.insert("tito", to);
    t.insert("toto", toto(1));
    t.insert_emplace("toto2", 2);
    t["tovo"] = toto(3);

    for(tree::iterator it = t.begin(); it != t.end(); ++it)
    {
        const toto & current = *it;
        std::cout << "iterator " << current.a << std::endl;
    }
    t.erase(t.begin());
    for(tree::const_iterator it = t.begin(); it != t.end(); ++it)
    {
        const toto & current = *it;
        std::cout << "iterator to const_iterator " << current.a << std::endl;
    }
    t.erase("toto2");
    const tree & ct = t;
    for(tree::const_iterator it = ct.begin(); it != ct.end(); ++it)
    {
        const toto & current = *it;
        std::cout << "const_iterator " << current.a << std::endl;
    }

    t.erase(t.cbegin());
    std::cout << ct.empty() << std::endl;
    t.clear();
    std::cout << t.empty() << std::endl;

    std::string c("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_");
    typedef generic_charset<char, size_t, 63, 128> ctoken_charset;
    ctoken_charset cs(c.begin(), c.end());

    prefix_tree<std::string, toto, ctoken_charset> tree2(cs);

    std::map<std::string, toto> m;
    m.clear();
    return 0;
}
