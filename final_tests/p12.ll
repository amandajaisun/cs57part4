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
  %8 = load i32, ptr %5, align 4
  %9 = mul i32 %8, 1
  store i32 %9, ptr %4, align 4
  store i32 0, ptr %3, align 4
  br label %11

10:                                               ; preds = %24
  %ret_load = load i32, ptr %7, align 4
  ret i32 %ret_load

11:                                               ; preds = %15, %1
  %12 = load i32, ptr %3, align 4
  %13 = load i32, ptr %6, align 4
  %14 = icmp slt i32 %12, %13
  br i1 %14, label %15, label %24

15:                                               ; preds = %11
  %16 = load i32, ptr %5, align 4
  call void @print(i32 %16)
  %17 = load i32, ptr %3, align 4
  %18 = add i32 %17, 1
  store i32 %18, ptr %3, align 4
  %19 = load i32, ptr %4, align 4
  store i32 %19, ptr %2, align 4
  %20 = load i32, ptr %5, align 4
  %21 = load i32, ptr %4, align 4
  %22 = add i32 %20, %21
  store i32 %22, ptr %4, align 4
  %23 = load i32, ptr %2, align 4
  store i32 %23, ptr %5, align 4
  br label %11

24:                                               ; preds = %11
  store i32 1, ptr %7, align 4
  br label %10
}

declare void @print(i32)
