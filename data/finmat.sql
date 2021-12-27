###!cnty_def - table with country definitions
CREATE TABLE IF NOT EXISTS cnty_def
(
    cnty_nm VARCHAR(5) NOT NULL PRIMARY KEY,
    cnty_full_nm VARCHAR(20) NOT NULL
);

###!ccy_def - table with currency definitions
CREATE TABLE IF NOT EXISTS ccy_def
(
    ccy_nm CHAR(3) NOT NULL PRIMARY KEY,
    ccy_full_nm VARCHAR(20),
    cnty_nm VARCHAR(5)
);

###!ccy_data - table with FX rates
CREATE TABLE IF NOT EXISTS ccy_data
(
    ccy_nm CHAR(3) NOT NULL,
    scn_no INT NOT NULL CHECK (scn_no > 0),
    rate FLOAT NOT NULL CHECK (rate > 0),
    FOREIGN KEY (ccy_nm) REFERENCES ccy_def(ccy_nm),
    UNIQUE (ccy_nm, scn_no)
);

###!load_ccy_data - load FX data
SELECT ccy_nm, scn_no, rate FROM ccy_data;

###!dcm_def - table holding day-count-method definitions
CREATE TABLE IF NOT EXISTS dcm_def
(
    dcm VARCHAR(10) NOT NULL PRIMARY KEY,
    description VARCHAR(100)
);

###!crv_def - table holding curve definitions
CREATE TABLE IF NOT EXISTS crv_def
(
    crv_nm VARCHAR(20) NOT NULL PRIMARY KEY,
    ccy_nm CHAR(3),
    dcm VARCHAR(10),
    crv_type VARCHAR(10) NOT NULL,
    underlying1 VARCHAR(20),
    underlying2 VARCHAR(20),
    FOREIGN KEY (ccy_nm) REFERENCES ccy_def(ccy_nm),
    FOREIGN KEY (dcm) REFERENCES fx_def(dcm),
    UNIQUE (crv_nm, ccy_nm)
);

###!crv_data - create table holding curve data
CREATE TABLE IF NOT EXISTS crv_data
(
    crv_nm VARCHAR(20) NOT NULL,
    scn_no INT NOT NULL CHECK (scn_no > 0),
    tenor INT NOT NULL CHECK (tenor > 0),
    rate FLOAT NOT NULL,
    FOREIGN KEY (crv_nm) REFERENCES crv_def(crv_nm)
    UNIQUE (crv_nm, scn_no, tenor)
);

###!load_all_crv_nms - load list of all curve names
SELECT crv_nm FROM crv_def;

###!load_crv_def - load curve definitions
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

###!load_base_crv_data - load curve data for a base curve
SELECT scn_no, tenor, rate FROM crv_data WHERE crv_nm = ##crv_nm##;

###!load_compound_crv_data - load curve data for a compound curve
SELECT
    crv1.scn_no,
    crv1.tenor,
    crv1.rate + crv2.rate AS rate 
FROM
    crv_data AS crv1
    INNER JOIN crv_data AS crv2 ON crv1.scn_no = crv2.scn_no AND crv1.tenor = crv2.tenor 
WHERE
    crv1.crv_nm = ##crv_nm1##
    AND crv2.crv_nm = ##crv_nm2##;

###!freq_def - table holding frequency definitions
CREATE TABLE IF NOT EXISTS freq_def
(
    freq VARCHAR(5) PRIMARY KEY
)
    
###!bnd_data - table holding bonds definitions
CREATE TABLE IF NOT EXISTS bnd_data
(
    ent_nm VARCHAR(5) NOT NULL,
    parent_id VARCHAR(10) NOT NULL,
    contract_id VARCHAR(10) NOT NULL,
    issuer_id VARCHAR(15),
    ptf VARCHAR(10) NOT NULL,
    account VARCHAR(50),
    isin VARCHAR(15),
    comments VARCHAR(256),
    bnd_type VARCHAR(10),
    fix_type VARCHAR(5),
    rtg VARCHAR(5),
    ccy_nm CHAR(3) NOT NULL,
    nominal FLOAT NOT NULL,
    deal_date INT NOT NULL,
    maturity_date INT NOT NULL,
    dcm VARCHAR(10),
    acc_int FLOAT,
    cpn_rate FLOAT,
    first_cpn_date INT,
    cpn_freq VARCHAR(5),
    first_fix_date INT,
    fix_freq VARCHAR(5),
    first_amort_date INT,
    amort_freq VARCHAR(5),
    amort FLOAT,
    rate_mult FLOAT,
    rate_add FLOAT,
    crv_disc VARCHAR(20) NOT NULL,
    crv_fwd VARCHAR(20),
    FOREIGN KEY (ccy_nm) REFERENCES ccy_def(ccy_nm),
    FOREIGN KEY (dcm) REFERENCES dcm_def(dcm),
    FOREIGN KEY (cpn_freq) REFERENCES freq_def(freq),
    FOREIGN KEY (fix_freq) REFERENCES freq_def(freq),
    FOREIGN KEY (amort_freq) REFERENCES freq_def(freq),
    FOREIGN KEY (crv_disc) REFERENCES crv_def(crv_nm),
    UNIQUE (ent_nm, parent_id, contract_id, ptf)
);

###!bnd_npv - table holding risk measures for bonds
CREATE TABLE IF NOT EXISTS bnd_npv
(
    scn_no INT NOT NULL,
    ent_nm VARCHAR(5) NOT NULL,
    parent_id VARCHAR(10) NOT NULL,
    contract_id VARCHAR(10) NOT NULL,
    ptf  VARCHAR(10) NOT NULL,
    acc_int FLOAT NOT NULL,
    npv FLOAT NOT NULL,
    acc_int_ref_ccy FLOAT NOT NULL,
    npv_ref_ccy FLOAT NOT NULL,
    FOREIGN KEY (ent_nm, parent_id, contract_id, ptf) REFERENCES bnd_def(ent_nm, parent_id, contract_id, ptf)
    UNIQUE (scn_no, ent_nm, parent_id, contract_id, ptf)
)

###!

