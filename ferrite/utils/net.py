from urllib.request import urlretrieve

class DownloadHook:
    def _format_bytes(self, num, suffix='B'):
        for unit in ["  ","Ki","Mi","Gi","Ti","Pi","Ei","Zi"]:
            if abs(num) < 1024.0:
                return f"{num:6.1f} {unit}B"
            num /= 1024.0
        return f"{num:.1f} YiB"

    def _progress_bar(self, progress):
        part = int(self.bar_length * progress)
        return "{}{}".format(
            self.bar_chars[1] * part,
            self.bar_chars[0] * (self.bar_length - part),
        )

    def __init__(self, bar_length=64, bar_chars=".#"):
        self.bar_length = bar_length
        self.bar_chars = bar_chars
        print("[{}] {}".format(
            self._progress_bar(0.0),
            self._format_bytes(0),
        ), end="")

    def __call__(self, block_count, block_size, total_size):
        if total_size <= 0:
            return
        
        size = block_count * block_size
        progress = size / total_size
        print("\r[{}] {} of {}".format(
            self._progress_bar(progress),
            self._format_bytes(size),
            self._format_bytes(total_size),
        ), end="")

def download(src_url, dst_path):
    urlretrieve(src_url, dst_path, DownloadHook())
    print()
