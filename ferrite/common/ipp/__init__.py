from __future__ import annotations

class Msg:
    def store(self) -> bytes:
        raise NotImplementedError
    
    @classmethod
    def load(data: bytes):
        raise NotImplementedError
