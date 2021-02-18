import os
from manage.tree import components
from manage.paths import BASE_DIR

def text_lines(*lines):
    return "".join([l + "\n" for l in lines])

def get_entry(task):
    name = task.name()
    stage = "build" if len(task.artifacts()) > 0 else "test"

    text = text_lines(
        f"{name}:",
        f"  stage: {stage}",
        f"  script:",
        f"    - python3 -u -m manage {name}",
    )

    if len(task.dependencies()) > 0:
        text += "  needs:\n"
        for dep in task.dependencies():
            text += f"    - {dep.name()}\n"

    if len(task.artifacts()) > 0:
        text += text_lines(
            "  artifacts:",
            "    paths:",
        )
        for art in task.artifacts():
            text += f"      - {os.path.relpath(art, BASE_DIR)}\n"

    return text

def fill_tree(tree, task):
    if task.name() in tree:
        return
    tree[task.name()] = task
    for dep in task.dependencies():
        fill_tree(tree, dep)

if __name__ == "__main__":
    end_tasks = [
        "all.build",
        "all.test_host",
    ]
    tree = {}
    for etn in end_tasks:
        cn, tn = etn.split(".")
        task = components[cn].tasks()[tn]
        fill_tree(tree, task)

    text = text_lines(
        "image: agerasev/debian-psc",
        "",
        "cache:",
        "  paths:",
        "    - target/download",
        "    - target/epics_base_*",
    )

    for task in tree.values():
        text += "\n"
        text += get_entry(task)

    print(text)
