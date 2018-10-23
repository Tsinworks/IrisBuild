#include <assert.h>
#include "vm/scope.h"
using namespace iris;
int main()
{
    scope s;
    s.set_value("test_value", value(), nullptr);
    const value* var = s.get_value("test_value");
    assert(var && var->type() == value_type::nil);

    value new_v;
    new_v.set_string("new_v");

    scope* new_scope = s.make_scope();
    new_scope->set_value("new_v", new_v, nullptr);

    value sv;
    sv.set_scope(new_scope);

    s.set_value("scope", sv, nullptr);

    const value* scope_va = s.get_value_in_scope("scope.new_v", nullptr);
    assert(scope_va && scope_va->str() == "new_v");
    return 0;
}