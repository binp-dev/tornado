from __future__ import annotations
from typing import Any, List, Optional, TypeVar, overload, Type as GenType

from random import Random

from numpy.typing import DTypeLike

from ferrite.codegen.base import Name, Source, TestInfo, Location, CONTEXT
from ferrite.codegen.utils import flatten, indent


class Type:

    def _debug_name(self) -> str:
        return f"{self.name}({type(self).__name__})"

    class NotImplemented(NotImplementedError):

        def __init__(self, owner: Type) -> None:
            super().__init__(owner._debug_name())

    @overload
    def __init__(self, name: Name, size: int) -> None:
        ...

    @overload
    def __init__(self, name: Name, size: None, min_size: int) -> None:
        ...

    def __init__(
        self,
        name: Name,
        size: Optional[int],
        min_size: Optional[int] = None,
    ) -> None:
        self.name = name
        if size is None:
            assert min_size is not None
            self._size = None
            self.min_size = min_size
        else:
            self._size = size
            self.min_size = size

    def is_sized(self) -> bool:
        return self._size is not None

    def is_empty(self) -> bool:
        return self._size == 0

    @property
    def size(self) -> int:
        if self._size is not None:
            return self._size
        else:
            raise self.NotImplemented(self)

    # Runtime

    def load(self, data: bytes) -> Any:
        raise self.NotImplemented(self)

    def store(self, value: Any) -> bytes:
        raise self.NotImplemented(self)

    def size_of(self, value: Any) -> int:
        return self.size

    def default(self) -> Any:
        raise self.NotImplemented(self)

    def random(self, rng: Random) -> Any:
        raise self.NotImplemented(self)

    def is_instance(self, value: Any) -> bool:
        raise self.NotImplemented(self)

    def __instancecheck__(self, value: Any) -> bool:
        return self.is_instance(value)

    def is_np(self) -> bool:
        return False

    def np_dtype(self) -> DTypeLike:
        raise self.NotImplemented(self)

    def np_shape(self) -> List[int]:
        raise self.NotImplemented(self)

    # Generation

    def c_type(self) -> str:
        raise self.NotImplemented(self)

    def c_size(self, obj: str) -> str:
        if self.size is not None:
            return str(self.size)
        else:
            raise self.NotImplemented(self)

    def _c_size_extent(self, obj: str) -> str:
        raise self.NotImplemented(self)

    def c_source(self) -> Optional[Source]:
        return None

    def rust_type(self) -> str:
        raise self.NotImplemented(self)

    def rust_size(self, obj: str) -> str:
        if self.size is not None:
            return f"<{self.rust_type()} as FlatSized>::SIZE"
        else:
            return f"<{self.rust_type()} as FlatBase>::size({obj})"

    def rust_source(self) -> Optional[Source]:
        return None

    def pyi_type(self) -> str:
        raise self.NotImplemented(self)

    def _pyi_np_dtype(self) -> str:
        raise self.NotImplemented(self)

    def pyi_source(self) -> Optional[Source]:
        return None

    # Tests

    def c_check(self, var: str, obj: Any) -> List[str]:
        raise self.NotImplemented(self)

    def c_object(self, obj: Any) -> str:
        raise self.NotImplemented(self)

    def rust_check(self, var: str, obj: Any) -> List[str]:
        raise self.NotImplemented(self)

    def rust_object(self, obj: Any) -> str:
        raise self.NotImplemented(self)

    def _make_test_objects(self, info: TestInfo) -> List[Any]:
        rng = Random(info.rng_seed)
        return [self.random(rng) for i in range(info.attempts)]

    def _c_test_name(self) -> str:
        return Name(CONTEXT.prefix, self.name, "test").snake()

    def c_test_source(self, info: TestInfo) -> Optional[Source]:
        if not self.is_empty():
            objs = self._make_test_objects(info)
            return Source(
                Location.TEST, [[
                    f"int {self._c_test_name()}(const uint8_t * const *data) {{",
                    *indent([
                        f"const {self.c_type()} *obj;",
                        *flatten(
                            [[
                                f"obj = (const {self.c_type()} *)(data[{i}]);",
                                *self.c_check(f"(*obj)", obj),
                            ] for i, obj in enumerate(objs)],
                            sep=[""],
                        ),
                        f"return 0;",
                    ]),
                    f"}}",
                ]],
                deps=[self.c_source()]
            )
        else:
            return self.c_source()

    def rust_test_source(self, info: TestInfo) -> Optional[Source]:
        objs = self._make_test_objects(info)
        return Source(
            Location.TEST, [[
                *([f"extern \"C\" {{ fn {self._c_test_name()}(data: *const *const u8) -> c_int; }}", f""]
                  if not self.is_empty() else []),
                f"#[test]",
                f"fn {self.name.snake()}() {{",
                *indent([
                    f"let data = vec![",
                    *indent(["&b\"" + "".join([f"\\x{b:02x}" for b in self.store(obj)]) + f"\"[..]," for obj in objs]),
                    f"];",
                    "",
                    f"assert_eq!(<{self.rust_type()}>::ALIGN, 1);",
                    "",
                    *flatten(
                        [[
                            f"let obj = <{self.rust_type()}>::from_bytes(&data[{i}]).unwrap().validate().unwrap();",
                            *self.rust_check(f"obj", obj),
                        ] for i, obj in enumerate(objs)],
                        sep=[""],
                    ),
                    *([
                        "", f"let raw_data = data.iter().map(|s| s.as_ptr()).collect::<Vec<_>>();",
                        f"assert_eq!(unsafe {{ {self._c_test_name()}(raw_data.as_ptr()) }}, 0, \"C test '{self._c_test_name()}' failed\");"
                    ] if not self.is_empty() else []),
                ]),
                f"}}",
            ]],
            deps=[self.rust_source()]
        )

    def self_test(self, info: TestInfo) -> None:
        for obj in self._make_test_objects(info):
            data = self.store(obj)
            assert self.store(self.load(data)) == data


Self = TypeVar("Self")


class Value:

    @classmethod
    def load(cls: GenType[Self], data: bytes) -> Self:
        raise NotImplementedError()

    def store(self) -> bytes:
        raise NotImplementedError()

    def size(self) -> int:
        raise NotImplementedError()
