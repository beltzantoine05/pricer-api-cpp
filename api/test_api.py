from fastapi.testclient import TestClient

from api.main import app

client = TestClient(app)
ATM = {"type": "call", "spot": 100, "strike": 100, "maturity": 1, "rate": 0.05}


def test_health():
    assert client.get("/health").json() == {"status": "ok"}


def test_bs_price_reference_value():
    r = client.post("/bs/price", json={**ATM, "vol": 0.2})
    assert r.status_code == 200
    assert abs(r.json()["price"] - 10.4506) < 1e-4


def test_greeks_fields():
    r = client.post("/bs/greeks", json={**ATM, "vol": 0.2}).json()
    assert set(r) == {"delta", "gamma", "vega", "theta", "rho"}
    assert 0 < r["delta"] < 1


def test_implied_vol_round_trip():
    price = client.post("/bs/price", json={**ATM, "vol": 0.3}).json()["price"]
    iv = client.post("/bs/implied-vol", json={**ATM, "price": price}).json()["implied_vol"]
    assert abs(iv - 0.3) < 1e-6


def test_mc_european_close_to_closed_form():
    r = client.post("/mc/european", json={**ATM, "vol": 0.2, "paths": 200_000}).json()
    assert abs(r["price"] - 10.4506) < 4 * r["std_error"]


def test_mc_asian_cheaper_than_european():
    r = client.post("/mc/asian", json={**ATM, "vol": 0.2, "steps": 12}).json()
    assert 0 < r["price"] < 10.4506


def test_validation_rejects_bad_input():
    assert client.post("/bs/price", json={**ATM, "vol": -0.2}).status_code == 422
    assert client.post("/bs/price", json={**ATM, "type": "cal", "vol": 0.2}).status_code == 422


def test_arbitrage_price_returns_422():
    r = client.post("/bs/implied-vol", json={**ATM, "price": 500})
    assert r.status_code == 422
    assert "no-arbitrage" in r.json()["detail"]
