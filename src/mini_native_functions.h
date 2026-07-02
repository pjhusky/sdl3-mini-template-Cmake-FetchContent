#ifndef MINI_NATIVE_C_FUNCS
#define MINI_NATIVE_C_FUNCS
namespace {
    static int nativeC_intAdd( const int a, const int b ) {

        const int result = a + b;
        // printf( "%s - %s: a = %d, b = %d, a + b = %d\n",  __FILE__, __FUNCTION__, a, b, result );
        printf( "NATIVE C/C++ - %s: a = %d, b = %d, a + b = %d\n", __FUNCTION__, a, b, result );
        return result;
    }
    static float nativeC_floatAdd( const float a, const float b ) {

        const float result = a + b;
        printf( "NATIVE C/C++ - %s: a = %f, b = %f, a + b = %f\n",  __FUNCTION__, a, b, result );
        return result;
    }
}
#endif

