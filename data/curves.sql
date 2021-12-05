###!1 - table holding curve definitions
CREATE TABLE IF NOT EXISTS crv_def
(
    crv_nm VARCHAR(20) UNIQUE NOT NULL,
    ccy_nm CHAR(3),
    dcm VARCHAR(10),
    crv_type VARCHAR(10) NOT NULL,
    underlying1 VARCHAR(20),
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

###!5 - load list of all curves
SELECT crv_nm FROM crv_def;

###!6 - load curve definition
SELECT
     crv1.crv_nm
    ,COALESCE(crv1.ccy_nm, crv2.ccy_nm) AS ccy_nm
    ,COALESCE(crv1.dcm, crv2.dcm) AS dcm
    ,crv1.crv_type
    ,crv1.underlying1
    ,crv1.underlying2 
FROM
    crv_def AS crv1
    LEFT JOIN crv_def AS crv2 ON crv1.underlying1 = crv2.crv_nm 
WHERE
    crv1.crv_nm = ##crv_nm##;

###!7 - load curve data for a base curve
SELECT scn_no, tenor, rate FROM crv_data WHERE crv_nm = ##crv_nm##;

###!8 - load curve data for a compound curve
SELECT crv1.scn_no, crv1.tenor, crv1.rate + crv2.rate AS rate FROM crv_data AS crv1 INNER JOIN crv_data AS crv2 ON crv1.scn_no = crv2.scn_no AND crv1.tenor = crv2.tenor WHERE crv1.crv_nm = ##crv_nm1## AND crv2.crv_nm = ##crv_nm2##;

###!
