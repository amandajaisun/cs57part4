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
  store i32 1, ptr %4, align 4
  store i32 0, ptr %3, align 4
  br label %9

8:                                                ; preds = %20
  ret i32 1

9:                                                ; preds = %13, %1
  %10 = load i32, ptr %3, align 4
  %11 = load i32, ptr %6, align 4
  %12 = icmp slt i32 %10, %11
  br i1 %12, label %13, label %20

13:                                               ; preds = %9
  %14 = load i32, ptr %5, align 4
  call void @print(i32 %14)
  %15 = load i32, ptr %3, align 4
  %16 = add i32 %15, 1
  store i32 %16, ptr %3, align 4
  %17 = load i32, ptr %4, align 4
  store i32 %17, ptr %2, align 4
  %18 = add i32 %14, %17
  store i32 %18, ptr %4, align 4
  %19 = load i32, ptr %2, align 4
  store i32 %19, ptr %5, align 4
  br label %9

20:                                               ; preds = %9
  store i32 1, ptr %7, align 4
  br label %8
}

declare void @print(i32)
