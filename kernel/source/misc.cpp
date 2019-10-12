#include <Sigma/common.h>
#include <Sigma/misc.h>
#include <klibc/string.h>
#include <klibc/stdio.h>

char* str_int;
uint64_t n_pairs;


void misc::kernel_args::init(char* str){
    n_pairs = 0;
    size_t len = strlen(str);
    for(uint64_t i = 0; i < len; i++){
        if(str[i] == ' ' || str[i] == '\0'){
            str[i] = '\0';
            n_pairs++;
        }
    }

    if(len != 0) n_pairs++; // Add last one if not empty
            
    str_int = str;
}

bool misc::kernel_args::get_bool(const char* key){
    char* _key = const_cast<char*>(key); // oOOH UB ooooh
    char* current = str_int;
    for(uint64_t i = 0; i < n_pairs; i++){
        if(memcmp(static_cast<void*>(current), static_cast<void*>(_key), misc::min(strlen(current), strlen(_key))) == 0)
            return true;
        
        current += strlen(current) + 1;
    }
    return false;
}

const char* misc::kernel_args::get_str(const char* key){
    char* _key = const_cast<char*>(key); // oOOH UB ooooh
    char* current = str_int;
    for(uint64_t i = 0; i < n_pairs; i++){
        printf("%s [%d] vs %s [%d]", current, strlen(current), _key, strlen(_key));
        if(memcmp(static_cast<void*>(current), static_cast<void*>(_key), misc::min(strlen(current), strlen(_key))) == 0)
            return current + strlen(_key) + 1; // Skip key + equal sign
        
        current += strlen(current) + 1;
    }
    return nullptr;
}