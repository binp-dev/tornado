from __future__ import annotations

from pathlib import Path

from ferrite.ci.gitlab.generate import Context, TaskJob, ScriptJob, generate_local, default_cache, write_to_file

if __name__ == "__main__":
    from example.components.tree import make_components

    self_dir = Path.cwd()
    ferrite_dir = self_dir.parent
    target_dir = self_dir / "target"

    ctx = Context("example", ferrite_dir)

    tasks = make_components(ferrite_dir, self_dir, target_dir).tasks()
    jobs = [
        ScriptJob("self_check", "mypy", [f"poetry run mypy -p {ctx.module}"], allow_failure=True),
        TaskJob("host_test", tasks["host.all.test"], []),
        TaskJob("cross_build", tasks["arm.all.build"], []),
        TaskJob("cross_build", tasks["aarch64.all.build"], []),
    ]

    text = generate_local(
        ctx,
        jobs,
        cache=[default_cache("example", lock_deps=True)],
        includes=[],
    )

    write_to_file(text, Path(__file__))

    print("Done.")
