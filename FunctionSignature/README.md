# IdentifyFunctionLoops 


Note:

Arrays can't be passed by value in C. They get demoted to pointers. So the called function sees a pointer to the array (passed by reference) and operates on it. If you want to make a copy, you have to either do so explicitly, or put your array inside a struct (which can be passed by value to functions.)
