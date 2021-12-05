###!1 - table holding FX rates
CREATE TABLE IF NOT EXISTS fx_data
(
    ccy_nm VARCHAR(3) UNIQUE NOT NULL,
    scn_no INT NOT NULL,
    rate FLOAT NOT NULL
);

###!2 - create index for FX data table
CREATE INDEX IF NOT EXISTS idx_fx_data ON fx_data (ccy_nm, scn_no);

###!3 - read all FX rates
SELECT * FROM fx_data;

###!
