class LookAhead():
    """
    Look ahead iterator. Look ahead using `.peek`.
    """
    _NONE = object()

    def __init__(self, iterable):
        self._it = iter(iterable)
        self._set_peek()

    def __iter__(self):
        return self

    def __next__(self):
        ret = self.peek
        self._set_peek()
        return ret

    def _set_peek(self):
        try:
            self.peek = next(self._it)
        except StopIteration:
            self.peek = self._NONE

    def __bool__(self):
        return self.peek is not self._NONE
