#!/bin/sh -x
CB_MAX_TRIES=1 ./testHarness.sh cases/1177_geometry_and_coords/
CB_MAX_TRIES=1 ./testHarness.sh cases/1677_geo_location_for_api_v2/
CB_MAX_TRIES=1 ./testHarness.sh cases/1789_patch_with_only_metadata
CB_MAX_TRIES=1 ./testHarness.sh cases/0000_ngsi10_geolocation_query/
CB_MAX_TRIES=1 ./testHarness.sh cases/1038_ngsiv2_geo_location_entities/
