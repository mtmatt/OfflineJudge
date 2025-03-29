# OfflineJudge

## 前言

當初我同學有在架設我們學校的線上評測系統，而一開始我們的系統非常不穩定，因此，他想到可以寫離線的程式答題系統緩解我們上課的需求。而我看到這個系統，我覺得這是一個不錯的想法，並依此加以改良，新增 TLE 判定和超時時會強制停止。

### 更新

為了可以被其他程式重複使用，於是我新增了更多的保護機制。例如移除所有全域變數，強制刪除 TLE 的進程。

## 使用(解題者)

出題者應該會提供已經編譯好的Run。請將你的答案寫入`Sulotion/Solve.cpp`，接著直接執行./Run即可。

### 使用其他程式語言

如果你使用的程式語言需要編譯，請使用命令列參數，以rust為例。

```
./Run true "rustc Solve.rs" "./Solution/Solve"
```

否則，你可以直接執行，所以我們用python為例。

```
./Run false "python ./Solution/Solve.py"
```

### 貼心小提醒

預設使用`make`編譯。

### 截圖

<img src="https://live.staticflickr.com/65535/54417317278_df7ec42ab4_h.jpg" alt="OfflineJudge"/>

## 使用(出題者)

TestCase 裡面存放有測試資料。裡面的檔案如下

| 檔案名稱 | 大致內容 |
| --- | --- |
| Gen.cpp | 預設的測試資料生成程式 |
| GraphGen.h | 圖論測資生成程式 ( [使用介紹](https://mtmatt.page/misc/the-test-case-builder-of-graph-theory/) ) |
| rng.h | 不是很優秀的隨機 |
| Sol.cpp | 官解檔(生完測資後可以移除) |
| log.txt | 測試資料們的基本資料 |

### Gen.cpp

介紹各個函式的功能

| 函式 | 功能 |
| --- | --- |
| solve() | 共用解答函式(會調用Sol.cpp) |
| SubTesk1(int a) | 生成第 a 筆測資 |

### Rng.h

| 函式 | 功能 |
| --- | --- |
| random_number_generater::operator(int n) | 生成隨機數，範圍 [1, n] |
| random_number_generater::operator(int a, int b) | 生成隨機數，範圍 [a, b] |
| random_number_generater::rand_char() | 隨機字母 |

```cpp
int seed = 1256;
random_number_generater rng(seed);
std::cout << rng(10) << " " << rng(10, 20) << " " << rng.rand_char();
```

### log.txt

只有三行，第一行是測試資料數目，第二行是執行時間限制，以毫秒為單位，第三行是題目ID。

```
TestCases:     5
TimeLimit(ms): 50
ProblemID: Default
```

## 主程式

| 函式 | 功能 |
| --- | --- |
| RunCode | 執行使用者程式(會計算時間，盡量減少多餘程式碼) |
| RunTestCase | 完成執行第 testCase 筆測試資料所需的前置條件 |
| Judge | 評判解答正確性 |
| RunSolution | 執行所有測試並輸出結果 |
| Encode | AC時回傳密碼(預設的加密演算法安全性很低，需要修改) |

## 競賽中不會出現的 C++ 功能

多執行續 thread 函式庫

```cpp
// 開新的執行續處理函式 RunCode
thread run{RunCode,timeLimit};
// 讓此執行續等待一段時間 chrono 格式
this_thread::sleep_for(chrono::milliseconds(timeLimit+20))
// 放空該執行續(與物件 run 取消綁定，但會繼續執行)
run.detach();
// 合併執行續，意味著它會等待到 run 那個執行序完成(return)後再合併
run.join();
```

## 其它題目

可以去雲端硬碟下載。這裡有一些題目。[Mega](https://mega.nz/folder/HgdU3J4A#NjDnWXqIS6Ov_hDGnrbtkA)
