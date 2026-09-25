# NOVA Language Pack (VS Code Extension v0.3.0)

ส่วนเสริม (extension) สำหรับ Visual Studio Code ที่เพิ่มการรองรับภาษา **NOVA** (`.nova`) อย่างสมบูรณ์แบบ

---

## ✨ ฟีเจอร์หลัก (Features)

### 1. 🎨 ไฮไลต์ไวยากรณ์ (Syntax Highlighting)
- รองรับคีย์เวิร์ดทั้งหมด: `let`, `if`, `elif`, `else`, `end`, `while`, `for`, `in`, `fn`, `return`, `break`, `continue`, `enum`, `try`, `catch`, `class`, `extends`, `static`, `self`, `super`, `import`, `as`, `global`, `new`, `type`, `typeof`
- รองรับ **f-string** (`f"Hello {name}!"`) พร้อมไฮไลต์นิพจน์ภายในวงเล็บปีกกา `{...}`
- รองรับ **Multiline string แบบ triple quotes** (`"""..."""`)
- รองรับ **List Comprehension** (`[x * 2 for x in list if x > 0]`)
- ไฮไลต์ตัวเลข (จำนวนเต็ม, ทศนิยม), Boolean, Null, Comments (`#`)
- ไฮไลต์ Standard Library Modules และ Built-in Functions

### 2. ⚡ การตรวจจับข้อผิดพลาดแบบเรียลไทม์ (Live Diagnostics)
- 🔴 **LexerError**: ตรวจจับ String หรือ Triple-quoted string ที่ไม่ปิด
- 🔴 **SyntaxError**:
  - บล็อกที่เปิดค้างไว้ (`if`, `while`, `for`, `fn`, `class`, `enum`, `try`) ที่ไม่มี `end` ปิด
  - การใช้ `break` หรือ `continue` นอกลูป `while` / `for`
  - วงเล็บ `()`, `[]`, `{}` ที่ไม่สมดุลหรือไม่ตรงคู่
- 🔴 **ReservedNameError**: การนำคำสงวน (เช่น `if`, `enum`, `self`) ไปตั้งเป็นชื่อตัวแปรหรือฟังก์ชัน
- 🔴 **ConstantError**: การพยายาม assign ค่าทับตัวแปรค่าคงที่ที่ประกาศด้วย `let`
- 🟡 **RuntimeError (Warning)**: การหารด้วยศูนย์ในโค้ดแบบคงที่ เช่น `/ 0`

### 3. 💡 ระบบช่วยพิมพ์อัจฉริยะ (IntelliSense & Autocomplete)
- แนะนำ Built-in Functions ทั้งหมดพร้อมรายละเอียด signature
- แนะนำฟังก์ชันของโมดูล Standard Library เมื่อพิมพ์จุด เช่น:
  - `file.` / `io.` → `read`, `write`, `append`, `exists`, `remove`, `lines`
  - `os.` / `sys.` → `platform`, `cwd`, `env`, `exit`, `exec`, `args`
  - `json.` → `parse`, `stringify`
  - `time.` → `now`, `sleep`, `clock`
  - `math.` → `sqrt`, `pow`, `floor`, `ceil`, `round`, `abs`, `sin`, `cos`, `tan`, `min`, `max`, `pi`, `e`
  - `random.` → `random`, `randint`, `choice`, `shuffle`, `uniform`, `range`
  - `string.` → `to_upper`, `to_lower`, `trim`, `starts_with`, `ends_with`, `contains`, `join`
  - `list.` → `contains`, `index_of`, `reverse`, `push`, `pop`, `len`
  - `dict.` → `has_key`
- แนะนำสมาชิกของ `enum` เช่น `Color.RED`, `Color.GREEN`
- แนะนำ Method และ Property ของ Class ทั้งแบบ `static` และผ่าน `self.`

### 4. 📖 ข้อมูลเอกสารเมื่อชี้เมาส์ (Hover Docs & Signature Help)
- ชี้เมาส์ที่ฟังก์ชัน, คลาส, enum หรือโมดูล เพื่อดู Type, Signature, รายชื่อสมาชิก และจุดที่ประกาศ
- แสดง **Parameter Hints** อัตโนมัติขณะพิมพ์พารามิเตอร์ของฟังก์ชัน

### 5. 🗺️ โครงสร้างไฟล์ (Outline & Document Symbols)
- แสดงแถบ Outline (Symbol Tree) ด้านข้าง แยก Class, Enum, Method, Function ให้คลิกข้ามไปยังส่วนต่างๆ ได้ทันที

### 6. 🚀 ปุ่มรันโค้ดทันใจ (Run Button)
- มีปุ่ม ▶ บนมุมขวาบนของ Editor สำหรับรันไฟล์ `.nova` ปัจจุบันผ่าน `nova.exe` บน Integrated Terminal ทันที
- มีระบบ Auto-detect ตัวรัน `nova.exe` อัตโนมัติ (จาก Program Directory หรือ Build Directory)

### 7. ⌨️ Code Snippets ครบครัน
- พิมพ์คีย์ลัดแล้วกด `Tab`:
  - `fn`, `class`, `classext`, `staticfn`, `enum`
  - `if`, `ifee`, `while`, `whilebreak`, `for`, `forrange`, `forbreak`
  - `listcomp`, `listcompif`
  - `fstr`, `tstr`
  - `importfile`, `importjson`, `importos`, `importtime`, `importmod`, `import`
  - `print`, `printf`, `try`, `let`

---

## 📦 วิธีการติดตั้ง

### ติดตั้งผ่านไฟล์ `.vsix`
1. ดาวน์โหลดหรือเปิดไฟล์ `nova-lang-0.3.0.vsix`
2. ใน Visual Studio Code ไปที่แท็บ **Extensions** (`Ctrl+Shift+X`)
3. คลิกปุ่มเมนู `...` (มุมขวาบนของแผง Extensions) → เลือก **Install from VSIX...**
4. เลือกไฟล์ `nova-lang-0.3.0.vsix`
