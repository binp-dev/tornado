from __future__ import annotations

from pathlib import Path

from ferrite.ci.gitlab.generate import main, default_variables, default_cache

from tornado.manage.tree import make_components

if __name__ == "__main__":

    end_tasks = [
        "host.all.test",
        "device.all.build",
    ]

    base_dir = Path.cwd()
    ferrite_dir = base_dir / "ferrite"
    target_dir = base_dir / "target"
    components = make_components(base_dir, ferrite_dir, target_dir)

    main(
        "tornado",
        base_dir,
        components,
        end_tasks,
        default_variables(),
        default_cache(lock_deps=True),
    )
