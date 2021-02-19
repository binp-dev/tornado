import os
from manage.tree import components
from manage.paths import BASE_DIR
from utils.strings import quote

def text_lines(*lines):
    return "".join([l + "\n" for l in lines])

def get_entry(task, stage, cache=False):
    name = task.name()

    text = text_lines(
        f"{name}:",
        f"  stage: {quote(stage)}",
        f"  script:",
        f"    - python3 -u -m manage {name}",
    )

    if len(task.dependencies()) > 0:
        text += "  needs:\n"
        for dep in task.dependencies():
            text += f"    - {dep.name()}\n"

    if len(task.artifacts()) > 0:
        text += text_lines(
            "  artifacts:" if not cache else "  cache:",
            "    paths:",
        )
        for art in task.artifacts():
            text += f"      - {os.path.relpath(art, BASE_DIR)}\n"

    return text

def fill_graph(graph, task, level=0):
    name = task.name()
    if name in graph:
        if graph[name][1] >= level:
            return
    graph[name] = (task, level)
    for dep in task.dependencies():
        fill_graph(graph, dep, level + 1)

if __name__ == "__main__":
    end_tasks = [
        "all.build",
        "all.test_host",
    ]
    cached = [
        "epics_base.",
        "_toolchain."
    ]

    graph = {}
    for etn in end_tasks:
        cn, tn = etn.split(".")
        task = components[cn].tasks()[tn]
        fill_graph(graph, task)
    sequence = [x for x in sorted(graph.values(), key=lambda x: -x[1])]
    stages = sorted(set([str(x[1]) for x in sequence]))

    text = text_lines(
        "image: agerasev/debian-psc",
        "",
        "cache:",
        "  paths:",
        "    - target/download",
        "    - target/epics_base_*",
    )

    text += "\nstages:\n"
    for s in reversed(stages):
        text += f"  - {quote(s)}\n"

    for task, stage in sequence:
        name = task.name()
        tc = any([p in name for p in cached])
        text += "\n"
        text += get_entry(task, str(stage), cache=tc)

    print(text)
