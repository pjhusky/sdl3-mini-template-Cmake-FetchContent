
// https://gitlab.sai.jku.at/booleguru/booleguru/-/blob/d06c03831598c1f6c1caeb201c339695bd806776/third_party/sol/sol2-3.3.0/examples/source/basic.cpp
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
// #include <sol/assert.hpp>

#include <fmt/format.h>


#ifdef __EMSCRIPTEN__
	#include <emscripten.h>
#endif

#include "mini_native_functions.h"



void mini_sol3_example() {
// create an empty lua state
	sol::state lua;





	// by default, libraries are not opened
	// you can open libraries by using open_libraries
	// the libraries reside in the sol::lib enum class
	lua.open_libraries(sol::lib::base);
	// you can open all libraries by passing no arguments
	// lua.open_libraries();

	// Register intAdd/floatAdd as global native functions callable from Lua
	lua.set_function("intAdd", &nativeC_intAdd);
	lua.set_function("floatAdd", &nativeC_floatAdd);



// lua.set_function("print", [](sol::variadic_args args) {
//     std::string out;
//     for (auto arg : args) { out += arg.as<std::string>() + " "; }
    
// #ifdef __EMSCRIPTEN__
//     // Bypasses stdout entirely and sends text directly to the native JS Engine console
//     EM_ASM({ console.log(UTF8ToString($0)); }, out.c_str());
// #else
//     std::cout << out << std::endl;
// #endif
// });

/*
lua.set_function("print", [](sol::variadic_args args) {
    std::string out;
    for (auto arg : args) { out += arg.as<std::string>() + " "; }
    
    //std::cout << out << std::endl;
	fmt::print("{}\n", out);
	// fmt::flush
});
*/

	// call lua code directly
	lua.script("print('1 hello world\\n')");

	// call lua code, and check to make sure it has loaded and
	// run properly:
	// auto handler = &sol::script_default_on_error;
	// lua.script("print('2 hello again, world')", handler);

	lua.script("print('2 hello again, world\\n')");


	// Use a custom error handler if you need it
	// This gets called when the result is bad
	auto simple_handler =
	     [](lua_State*, sol::protected_function_result result) {
		     // You can just pass it through to let the
		     // call-site handle it
		     return result;
	     };
	// the above lambda is identical to sol::simple_on_error,
	// but it's shown here to show you can write whatever you
	// like

	//
	{
		auto result = lua.script(
		     "print('3 hello hello again, world\\n') \n return 24",
		     simple_handler);
		if (result.valid()) {
			std::cout << "the third script worked, and a "
			             "double-hello statement should "
			             "appear above this one!"
			          << std::endl;
			int value = result;
			//!!! sol_c_assert(value == 24);
		}
		else {
			std::cout << "the third script failed, check the "
			             "result type for more information!"
			          << std::endl;
		}
	}

	{
		auto result
		     = lua.script("does.not.exist", simple_handler);
		if (result.valid()) {
			std::cout << "the fourth script worked, which it "
			             "wasn't supposed to! Panic!"
			          << std::endl;
			int value = result;
			//!!! sol_c_assert(value == 24);
		}
		else {
			sol::error err = result;
			std::cout << "the fourth script failed, which "
			             "was intentional!\t\nError: "
			          << err.what() << std::endl;
		}
	}

	{
		std::cout << "\n--- Running Call-Native-C Script ---\n";
		auto result = lua.script(
		     "local resFloat = floatAdd(18.5, 44.25) \n"
			 "print(\"lua>> resFloat = \" .. resFloat) \n"
		     "local resInt = intAdd(14, 22) \n"
			 "print(\"lua>> resInt = \" .. resInt) \n"
		     "return resInt",
		     simple_handler);
		if (result.valid()) {
			int value = result;
			std::cout << "sol3/lua execution success. intAdd(14, 22) returned: " << value << std::endl;
		}
		else {
			sol::error err = result;
			std::cout << "Call-Native-C script failed! Error: " << err.what() << std::endl;
		}
	}

	std::cout << std::endl;
}
