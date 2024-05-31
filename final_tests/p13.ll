target triple = "x86_64-pc-linux-gnu"

define i32 @0(i32 %0) {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  store i32 %0, ptr %6, align 4
  store i32 1, ptr %5, align 4
  %8 = load i32, ptr %6, align 4
  %9 = load i32, ptr %5, align 4
  %10 = mul i32 %8, %9
  store i32 %10, ptr %4, align 4
  store i32 0, ptr %3, align 4
  br label %12

11:                                               ; preds = %25
  %ret_load = load i32, ptr %7, align 4
  ret i32 %ret_load

12:                                               ; preds = %16, %1
  %13 = load i32, ptr %3, align 4
  %14 = load i32, ptr %6, align 4
  %15 = icmp slt i32 %13, %14
  br i1 %15, label %16, label %25

16:                                               ; preds = %12
  %17 = load i32, ptr %5, align 4
  call void @print(i32 %17)
  %18 = load i32, ptr %3, align 4
  %19 = add i32 %18, 1
  store i32 %19, ptr %3, align 4
  %20 = load i32, ptr %4, align 4
  store i32 %20, ptr %2, align 4
  %21 = load i32, ptr %5, align 4
  %22 = load i32, ptr %4, align 4
  %23 = add i32 %21, %22
  store i32 %23, ptr %4, align 4
  %24 = load i32, ptr %2, align 4
  store i32 %24, ptr %5, align 4
  br label %12

25:                                               ; preds = %12
  %26 = load i32, ptr %5, align 4
  store i32 %26, ptr %7, align 4
  br label %11
}

declare void @print(i32)
