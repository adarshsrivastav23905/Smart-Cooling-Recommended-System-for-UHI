import json
import threading
import unittest
from http.server import HTTPServer
from urllib.error import HTTPError
from urllib.request import Request, urlopen

from dashboard import server as dashboard_server


class QuietDashboardHandler(dashboard_server.DashboardHandler):
    def log_message(self, format, *args):
        pass


class DashboardApiTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.http_server = HTTPServer(("127.0.0.1", 0), QuietDashboardHandler)
        cls.server_thread = threading.Thread(target=cls.http_server.serve_forever)
        cls.server_thread.start()
        cls.base_url = f"http://127.0.0.1:{cls.http_server.server_port}"

    @classmethod
    def tearDownClass(cls):
        cls.http_server.shutdown()
        cls.server_thread.join()
        cls.http_server.server_close()

    def get_json(self, path):
        try:
            response = urlopen(f"{self.base_url}{path}")
        except HTTPError as error:
            with error:
                return error.code, json.loads(error.read())
        with response:
            return response.status, json.loads(response.read())

    def post_json(self, path, payload):
        request = Request(
            f"{self.base_url}{path}",
            data=json.dumps(payload).encode("utf-8"),
            headers={"Content-Type": "application/json"},
            method="POST",
        )
        try:
            response = urlopen(request)
        except HTTPError as error:
            with error:
                return error.code, json.loads(error.read())
        with response:
            return response.status, json.loads(response.read())

    def test_zone_list_and_lookup(self):
        status, listing = self.get_json("/api/v1/zones")
        self.assertEqual(status, 200)
        self.assertGreater(listing["count"], 0)
        self.assertIn("record_count", listing["zones"][0])

        status, zone = self.get_json("/api/v1/zones/MZ_001")
        self.assertEqual(status, 200)
        self.assertEqual(zone["zone_id"], "MZ_001")

    def test_unknown_zone_returns_not_found(self):
        status, result = self.get_json("/api/v1/zones/UNKNOWN")
        self.assertEqual(status, 404)
        self.assertIn("error", result)

    def test_recommendation_and_scenario(self):
        status, recommendation = self.post_json(
            "/api/v1/recommendations", {"zone_id": "MZ_004"}
        )
        self.assertEqual(status, 200)
        self.assertEqual(recommendation["mode"], "local_rules")
        self.assertIn("recommendation_id", recommendation)

        status, scenario = self.post_json(
            "/api/v1/scenarios",
            {"zone_id": "MZ_004", "canopy_pct": 25, "area_sqm": 50000},
        )
        self.assertEqual(status, 200)
        self.assertGreater(scenario["scenario"]["estimated_temperature_drop_c"], 0)
        self.assertGreater(scenario["scenario"]["estimated_budget_inr"], 0)

    def test_scenario_rejects_out_of_range_values(self):
        status, result = self.post_json(
            "/api/v1/scenarios", {"zone_id": "MZ_004", "canopy_pct": 100}
        )
        self.assertEqual(status, 400)
        self.assertIn("canopy_pct", result["error"])


if __name__ == "__main__":
    unittest.main()
