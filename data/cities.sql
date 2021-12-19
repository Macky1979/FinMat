###!create_tbl
CREATE TABLE IF NOT EXISTS cities (city VARCHAR(20), country VARCHAR(20));

###!delete_tbl
DELETE FROM cities;

###!insert_into_tbl
INSERT INTO cities (city, country) VALUES ('Prague', 'Czech Republic');

###!select_from_tbl
SELECT * FROM cities;

###!
