#ifndef ZETA_SINGLETON
#define ZETA_SINGLETON

template<typename T>
class singleton {
    public:
        static T& getInstance(){
            static T instance;
            static bool constructed = false;
            if(!constructed) instance = T();
            return instance;
        }

        singleton(singleton const&) = delete;
        void operator=(singleton const&) = delete;
    private:
        singleton() {}
};


#endif