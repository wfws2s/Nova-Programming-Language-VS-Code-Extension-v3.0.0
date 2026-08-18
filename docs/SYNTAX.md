# NOVA — Syntax Specification (v0.2.2)

> **Status:** Complete syntax specification for NOVA v0.2.2.

---

## 1. Variables, Constants, and Scopes

### Mutable and Immutable Bindings
```nova
x = 10              # Mutable variable declaration / assignment
x = 20              # Reassignment
let pi = 3.14159    # Immutable constant declaration (let)
```
Reassigning a `let` binding raises `ConstantError`.

### Global Variables
Inside functions or methods, use `global <var>` to bind assignments to global scope:
```nova
score = 0

fn add_points(pts)
    global score
    score = score + pts
    let local_status = "updated"
end

add_points(50)
print(score)        # 50
```

---

## 2. Object-Oriented Programming (Classes)

### Class Definition & Instantiation
Classes are defined with `class Name ... end`. Constructors are defined with `fn init(self, ...)`:

```nova
class Person
    fn init(self, name, age)
        self.name = name
        self.age = age
    end

    fn greet(self)
        return "Hi, I am " + self.name + " (" + string(self.age) + ")"
    end

    fn have_birthday(self)
        self.age = self.age + 1
    end
end

let p = Person("Alice", 25)
print(p.greet())           # Hi, I am Alice (25)
p.have_birthday()
print(p.age)               # 26
p.hobby = "Photography"    # Dynamic property assignment
print(p.hobby)             # Photography
```

### Inheritance & `super`
Inherit from a base class with `extends` (or `:`):

```nova
class Animal
    fn init(self, name)
        self.name = name
    end

    fn speak(self)
        return self.name + " makes a sound"
    end
end

class Dog extends Animal
    fn init(self, name, breed)
        super.init(name)
        self.breed = breed
    end

    fn speak(self)
        return self.name + " (" + self.breed + ") barks!"
    end

    fn parent_sound(self)
        return super.speak()
    end
end

let d = Dog("Buddy", "Golden Retriever")
print(d.speak())         # Buddy (Golden Retriever) barks!
print(d.parent_sound())  # Buddy makes a sound
```

### Static Methods
Define static methods using `static fn`:

```nova
class MathHelper
    static fn square(x)
        return x * x
    end

    static fn add(a, b)
        return a + b
    end
end

print(MathHelper.square(7))    # 49
print(MathHelper.add(10, 20))  # 30
```

---

## 3. Modules and External File Imports

NOVA allows importing other `.nova` files via direct relative or absolute paths, with aliasing via `as`:

```nova
# Direct path import with alias
import "./math_helper.nova" as helper
import "../external/utils.nova" as utils
import "C:/Nova/shared/lib.nova" as shared_lib

# Standard library module imports
import random as rnd
import math
```

### Calling Imported Functions and Classes
```nova
let calc = helper.Calculator("Model-X")
print(calc.compute(10, 20, "+"))
print(helper.power(2, 5))
```

---

## 4. Type Inspection (`type`, `typeof`, `is`)

Check the data type of any variable or expression:

```nova
let num = 42
let text = "Hello"
let list_val = [1, 2, 3]
let dog = Dog("Rex", "Labrador")

# Functional syntax
print(type(num))          # "Number"
print(type(text))         # "String"
print(type(list_val))     # "List"
print(type(dog))          # "Instance"
print(type(Dog))          # "Class"

# Keyword expression syntax
print(type num)           # "Number"
print(typeof(num))        # "Number"

# 'is' operator type checking
if dog is "Instance"
    print("dog is a class instance")
end
```

---

## 5. Standard Library `random`

```nova
import random as rnd

# Random float in [0.0, 1.0)
let r = rnd.random()

# Random integer in [min, max] inclusive
let roll = rnd.randint(1, 6)

# Random item picked from list
let colors = ["red", "green", "blue", "yellow"]
let chosen = rnd.choice(colors)

# Shuffle list in-place
let deck = [1, 2, 3, 4, 5]
rnd.shuffle(deck)

# Random float in range [min, max]
let val = rnd.uniform(10.0, 20.0)

# Random integer in range [start, stop)
let step = rnd.range(0, 100)
```

---

## 6. Collections and Slicing

```nova
# Lists
items = [10, 20, 30, 40, 50]
print(items[0])       # 10
print(items[-1])      # 50

# Slicing
print(items[1..4])    # [20, 30, 40]
print(items[..3])     # [10, 20, 30]
print(items[-2..])    # [40, 50]

# List functions
append(items, 60)
let popped = pop(items)

# Dictionaries
user = {
    "name": "Nova User",
    "role": "Developer",
}
print(user["name"])
user["active"] = true
print(keys(user))     # ["name", "role", "active"]
print(values(user))   # ["Nova User", "Developer", true]
```

---

## 7. Control Flow

### `if`, `elif`, `else`
```nova
if score >= 90
    print("Grade: A")
elif score >= 80
    print("Grade: B")
elif score >= 70
    print("Grade: C")
else
    print("Grade: F")
end
```

### `while` Loop
```nova
i = 0
while i < 5
    print("Count:", i)
    i = i + 1
end
```

### `for` Loop
```nova
for item in [10, 20, 30]
    print(item)
end

for num in 1..5
    print(num)
end
```

---

## 8. Functions & Closures

```nova
fn multiply(a, b)
    return a * b
end

fn make_adder(x)
    fn adder(y)
        return x + y
    end
    return adder
end

let add10 = make_adder(10)
print(add10(5))       # 15
```

---

## 9. Error Handling (`try ... catch`)

```nova
try
    let result = 10 / 0
catch err
    print("Caught error:", err["type"], err["message"])
    print(err["stack"])
end
```
