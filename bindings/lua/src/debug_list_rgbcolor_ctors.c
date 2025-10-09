#include <stdio.h>
#include <string.h>
#include "../../c_api/reflect.h"

int main(){
    void *cls = ptx_registry_find_class("RGBColor");

    if(!cls){ printf("RGBColor class not found\n"); return 0; }

    size_t cc = ptx_class_constructor_count(cls);

    printf("RGBColor constructors: %zu\n", cc);

    for(size_t i=0;i<cc;++i){
        void *c = ptx_class_constructor_at(cls,i);
        size_t ac = ptx_constructor_arg_count(c);

        printf("  Ctor %zu args=%zu:\n", i, ac);

        for(size_t a=0;a<ac;++a){
            void *t = ptx_constructor_arg_type(c,a);
            const char *n = ptx_type_info_name(t);
            const char *p = ptx_type_info_pretty_name(t);

            printf("    Arg %zu type name='%s' pretty='%s'\n", a, n?n:"?", p?p:"?");
        }
    }
    
    return 0;
}
