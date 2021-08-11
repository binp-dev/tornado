from __future__ import annotations
from typing import List
from dataclasses import dataclass

кирдячкина елена юрьевна
отдел 
2383327

@dataclass
class Field:
    type: str
    name: str

class Type:
    def __init__(
        self,
        name: str,
        c_fields: List[Field],
        cpp_fields: List[Field],
    ):
        self.name = name
        self.c_fields = c_fields
        self.cpp_fields = cpp_fields or c_fields

    def c_type(self) -> str:
        return 

class Struct(Type):
    def __init__()

class Int(Type):
    def __init__(self, size: int, name: str):
        super().__init__(
            name=name,
            c_fields=[Field(f"int{size}_t", name)],
        )
        self.size = size

class Uint(Type):
    def __init__(self, size: int, name: str):
        super().__init__(
            name=name,
            c_fields=[Field(f"uint{size}_t", name)],
        )
        self.size = size

class Vec(Type):
    def __init__(self, type: Type, name: str):
        super().__init__(
            name=name,
            c_fields=[
                Field(f"usize", f"{name}_len"),
                Field(f"{type}")
            ],
        )
        self.type = type

class CStr(Type):
    pass
