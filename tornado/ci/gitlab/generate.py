from __future__ import annotations

from pathlib import Path

from ferrite.ci.gitlab.generate import Context, ScriptJob, TaskJob, default_variables, default_cache, generate, write_to_file

if __name__ == "__main__":
    from tornado.components.tree import make_components

    self_dir = Path.cwd()
    ferrite_dir = self_dir / "ferrite"
    target_dir = self_dir / "target"

    ctx = Context("ferrite", self_dir)

    tasks = make_components(self_dir, ferrite_dir, target_dir).tasks()
    jobs = [
        ScriptJob("self_check", "mypy", [f"poetry run mypy -p {ctx.module}"], allow_failure=True),
        TaskJob("host_test", tasks["host.all.test"], []),
        #TaskJob("cross_build", tasks["device.all.build"], []),
    ]

    text = generate(
        ctx,
        jobs,
        cache=[default_cache(ctx.module, lock_deps=True)],
        includes=[],
        vars=default_variables(),
        image_version="0.3",
    )

    write_to_file(text, Path(__file__))

    print("Done.")
