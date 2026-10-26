"""Validate a clean apply, idempotence and wrong-revision refusal in a temp clone."""
import pathlib, subprocess, sys, tempfile
root=pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as tmp:
    tree=pathlib.Path(tmp)/"zmk"
    subprocess.run(["git","clone","--shared","--no-checkout",sys.argv[1],str(tree)],check=True)
    subprocess.run(["git","-C",str(tree),"checkout","4dd95d7840eb3d5a3f59bb29fbf1ab054715ef85"],check=True)
    cmd=[sys.executable,str(root/"scripts/apply_bridge.py"),str(tree)]
    subprocess.run(cmd,check=True)
    before=subprocess.check_output(["git","-C",str(tree),"diff"])
    subprocess.run(cmd,check=True)
    assert before==subprocess.check_output(["git","-C",str(tree),"diff"])
    subprocess.run([sys.executable,str(root/"tests/test_bridge.py"),str(tree)],check=True)
    handler=(tree/"app/src/split/central.c").read_text()
    assert handler.index("source >= ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT") < handler.index("switch (ev.type)")
    subprocess.run(["git","-C",str(tree),"-c","user.name=Test","-c","user.email=test@example.invalid",
                    "commit","--allow-empty","-m","unsupported revision"],check=True)
    assert subprocess.run(cmd).returncode != 0
print("PASS: clean patch, idempotence, bounds guard, wrong revision rejected")
