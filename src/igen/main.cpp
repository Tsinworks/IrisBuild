#include "parse/xige.h"
#include "gen/cxx_gen.h"

void b(node_t n, node_type t, void* a)
{
    ((Node::Visitor*)a)->Begin((Node*)n, Node::Type(t));
}

void e(node_t n, void* a)
{
    ((Node::Visitor*)a)->End((Node*)n);
}

int main(int argc, char* argv[]) {
    if (argv[1]) {
        CxxVisitor v;
        node_load_callback(argv[1], b, e, nullptr, &v);
        auto info = v.Dump();
    }
    return 0;
}