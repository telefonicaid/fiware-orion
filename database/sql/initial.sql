CREATE TYPE ValueType AS ENUM(
    'String',
    'Number',
    'Boolean',
    'Relationship',
    'Compound',
    'DateTime',
    'GeoPoint',
    'GeoPolygon',
    'GeoMultiPolygon',
    'GeoLineString',
    'GeoMultiLineString',
    'LanguageMap');

CREATE TYPE OperationMode AS ENUM(
    'Create',
    'Append',
    'Update',
    'Replace',
    'Delete');

CREATE TABLE IF NOT EXISTS entities (
    instanceId TEXT PRIMARY KEY,
     ts TIMESTAMP NOT NULL,
     opMode OperationMode,
     id TEXT NOT NULL,
     type TEXT NOT NULL);

CREATE TABLE IF NOT EXISTS attributes (
    instanceId TEXT PRIMARY KEY,
    id TEXT NOT NULL,
    opMode OperationMode,
    entityId TEXT NOT NULL,
    observedAt TIMESTAMP,
    subProperties BOOL,
    unitCode TEXT,
    datasetId TEXT,
    valueType ValueType,
    text TEXT,
    boolean BOOL,
    number FLOAT8,
    datetime TIMESTAMP,
    compound JSONB,
    geoPoint GEOGRAPHY(POINTZ, 4326),
    geoPolygon GEOGRAPHY(POLYGON, 4326),
    geoMultiPolygon GEOGRAPHY(MULTIPOLYGON, 4326),
    geoLineString GEOGRAPHY(LINESTRING, 4326),
    geoMultiLineString GEOGRAPHY(MULTILINESTRING, 4326),
    ts TIMESTAMP NOT NULL);

CREATE TABLE IF NOT EXISTS subAttributes (
    instanceId TEXT PRIMARY KEY,
    id TEXT NOT NULL,
    entityId TEXT NOT NULL,
    attrInstanceId TEXT NOT NULL,
    observedAt TIMESTAMP,
    unitCode TEXT,
    valueType ValueType,
    text TEXT,
    boolean BOOL,
    number FLOAT8,
    datetime TIMESTAMP,
    compound JSONB,
    geoPoint GEOGRAPHY(POINTZ, 4326),
    geoPolygon GEOGRAPHY(POLYGON, 4326),
    geoMultiPolygon GEOGRAPHY(MULTIPOLYGON, 4326),
    geoLineString GEOGRAPHY(LINESTRING, 4326),
    geoMultiLineString GEOGRAPHY(MULTILINESTRING, 4326),
    ts TIMESTAMP NOT NULL);
