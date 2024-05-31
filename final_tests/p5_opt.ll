target triple = "x86_64-pc-linux-gnu"

define i32 @0(i32 %0) {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  store i32 %0, ptr %4, align 4
  store i32 10, ptr %3, align 4
  %6 = load i32, ptr %4, align 4
  %7 = add i32 10, %6
  store i32 %7, ptr %2, align 4
  %8 = load i32, ptr %2, align 4
  store i32 %8, ptr %5, align 4
  br label %9

9:                                                ; preds = %1
  %ret_load = load i32, ptr %5, align 4
  ret i32 %ret_load
}
