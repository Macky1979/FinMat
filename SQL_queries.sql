##!1
CREATE TABLE IF NOT EXISTS cities (city VARCHAR(20), country VARCHAR(20));

##!2
DELETE FROM cities;

##!3
INSERT INTO cities (city, country) VALUES ('Prague', 'Czech Republic');

##!4
SELECT * FROM cities;

##!
