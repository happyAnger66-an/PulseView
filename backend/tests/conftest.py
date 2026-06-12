import os
import sys
from pathlib import Path

import pytest

BACKEND_ROOT = Path(__file__).resolve().parent.parent
if str(BACKEND_ROOT) not in sys.path:
    sys.path.insert(0, str(BACKEND_ROOT))

TEST_MCAP = Path(
    os.environ.get(
        "PULSEVIEW_TEST_MCAP",
        str(BACKEND_ROOT.parent.parent / "test2" / "test2_0.mcap"),
    )
)


@pytest.fixture()
def data_dir(tmp_path, monkeypatch):
    monkeypatch.setenv("PULSEVIEW_DATA_DIR", str(tmp_path))
    return tmp_path


@pytest.fixture()
def client(data_dir):
    import importlib

    import app.main as main_mod
    import app.store as store_mod

    importlib.reload(store_mod)
    importlib.reload(main_mod)
    from fastapi.testclient import TestClient

    return TestClient(main_mod.app)


@pytest.fixture()
def mcap_path():
    if not TEST_MCAP.exists():
        pytest.skip(f"missing test mcap: {TEST_MCAP}")
    return TEST_MCAP
