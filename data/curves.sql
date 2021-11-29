###!1 - table holding curve definitions
CREATE TABLE IF NOT EXISTS crv_def
(
    crv_nm VARCHAR(20) UNIQUE NOT NULL,
    ccy_nm CHAR(3),
    dcm VARCHAR(10),
    crv_type VARCHAR(10) NOT NULL,
    underlying1 VARCHAR(20) NOT NULL,
    underlying2 VARCHAR(20)
);

###!2 - create index for curve definition table
CREATE INDEX IF NOT EXISTS idx_crv_def ON crv_def (crv_nm, ccy_nm);

###!3 - create table holding curve data
CREATE TABLE IF NOT EXISTS crv_data
(
    crv_nm VARCHAR(20) NOT NULL,
    scn_no INT NOT NULL,
    tenor INT NOT NULL,
    rate FLOAT NOT NULL,
    UNIQUE(crv_nm, tenor)
);

###!4 - create index for curve data table
CREATE INDEX IF NOT EXISTS idx_crv_data ON crv_data (crv_nm, scn_no, tenor);

###!5 - load curve definition
SELECT * FROM crv_def WHERE crv_nm = ##crv_nm##;

###!6 - load curve data for a base curve
SELECT scn_no, tenor, rate FROM crv_data WHERE crv_nm = ##crv_nm##;

###!7 - load curve data for a compound curve
SELECT crv1.scn_no, crv1.tenor, crv1.rate + crv2.rate AS rate FROM crv_data AS crv1 INNER JOIN crv_data AS crv2 ON crv1.scn_no = crv2.scn_no AND crv1.tenor = crv2.tenor WHERE crv1.crv_nm = ##crv_nm1## AND crv2.crv_nm = ##crv_nm2##;

###!
