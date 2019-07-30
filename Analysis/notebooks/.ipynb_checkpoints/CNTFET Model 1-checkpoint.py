import numpy as np
from matplotlib import pyplot as plt

def coth(x):
    return np.cosh(x)/np.sinh(x)






def CNTCarrierApprox(d, Ef, T):
    acc = 0.142e-9
    Vp = 3
    Eg = 2 * Vp * acc / d
    Evh = Eg / 2
    kB = 1.38e-23
    q = 1.6e-19
    vt = kB * (T + 273.15) / q

    x = np.exp((Evh - Ef) / vt)
    n0 = 8 / 3 / np.pi / acc / Vp
    n1 = n0 * np.sqrt(np.pi * vt / 2) * np.sqrt(Evh) / x
    a0 = 1 / (1 + x)
    a1 = -x / vt / (1 + x) ** 2
    _del = Ef + vt * np.log(1 + x)
    d2_E2 = np.sqrt(_del ** 2 - Evh ** 2)
    n2 = n0 * ((a1 / 2 * _del + a0 - a1 * Evh) * d2_E2 + a1 / 2 * Evh ** 2 * np.log((_del + d2_E2) / Evh))
    fsmo = 1 / (1 + x ** (-1.3))
    n = n1 * fsmo + n2 * (1 - fsmo)

    #%%% 2nd subband
    Evh = Eg
    x = np.exp((Evh - Ef) / vt)
    n0 = 8 / 3 / np.pi / acc / Vp
    n1 = n0 * np.sqrt(np.pi * vt / 2) * np.sqrt(Evh) / x
    a0 = 1 / (1 + x)
    a1 = -x / vt / (1 + x) ** 2
    _del = Ef + vt * np.log(1 + x)
    n2 = n0 * ((a1 / 2 * _del + a0 - a1 * Evh) * np.sqrt(_del ** 2 - Evh ** 2) + a1 / 2 * Evh ** 2 * np.log((_del + np.sqrt(_del ** 2 - Evh ** 2)) / Evh))
    fsmo = 1 / (1 + x ** (-1.3))
    n = n + (n1 * fsmo + n2 * (1 - fsmo))
    
    return n

# Scalelength_GAA: scale length in a GAA cylindrical structure
#
# Input:
#   kch:    dielectric constant of CNT
#   kox:    dielectric constant of gate oxide
#   d:      CNT diameter [m]
#   tox:    gate oxide thickness [m]
#
# Output:
#   lambda: scale length [m]
def Scalelength_GAA(kch, kox, d, tox):
    z0 = 2.4048
    delta = d * z0 / (d + 2 * tox)
    b = 0.4084 * (delta / 2 - delta ** 3 / 16) / (2 / np.pi / delta)
    #     b = 0.4084*(delta/2-delta^3/16+delta^5/384)/(2/np.pi/delta);
    _lambda = (d / 2 + tox) / z0 * (1 + b * (kch / kox - 1))
    return _lambda


# RcCNT: resistance in the source/drain extension
#
# Input:
#   Lsd:    S/D extension length [m]
#   nsd:    doping density [1/m]
#   d:      CNT diameter [m]
#
# Output:
#   R:      single-sided resistance [Ohm]
# 
# Comment:
#   This is an empirical function fitted to low-bias Landauer formula
def RextCNT(Lsd, nsd, d):
    Rsd0 = 35
    npow = 2.1
    dpow = 2
    R = Rsd0 * (Lsd * 1e9) / (nsd / 1e9) ** npow / (d * 1e9) ** dpow
    return R


# RcCNT: CNT/Metal contact resistance
#
# Input:
#   d:      CNT diameter [m]
#   Lc:     contact length [m]
#   rcmod:  0 - Rc = Rq/2
#           1 - diameter dependent
#           2 - diameter independent
#
# Output:
#   Rc:     single-sided CNT/Metal contact resistance [Ohm]
#   Lt:     transfer length [m]
# 
# Comment:
#   The model is based on the transmission line model in Ref. 1. The
#   diameter dependence is calibrated to experimental data from Ref. 2.
#   Ref. 1: Chen, "The Role of Metal-Nanotube Contact in the Performance of
#   Carbon Nanotube Field-Effect Transistors," Nano Lett., 2005
#   Ref. 2: Solomon, "Contact Resistance to a One-Dimensional
#   Quasi-Ballistic Nanotube/Wire," EDL, 2011
def RcCNT(Lc, d, rcmod):
    # physical constants
    h = 6.626e-34
    q = 1.6e-19
    Rq = h / (4 * q ** 2)
    if rcmod == 0:
        Rc = Rq / 2
        Lt = 0
    else:
        mfp = 380e-9
        if rcmod == 2:
            gc = 2e3
        else:
            #             gco     = 230;  % d = 1.3 nm as reference
            gco = 490        # d = 1.2 nm as reference
            E00 = 0.032
            phims = 0.4
            acc = 0.142e-9
            Vp = 3
            Eg = 2 * Vp * acc / d
            phib = Eg / 2 - phims
            gc = gco * np.exp(-phib / E00)
        
        alpha = 1. / mfp
        beta = 0.5 * gc * Rq
        gamma = np.sqrt(2. * alpha * beta + beta ** 2)
        Rc = gamma * Rq / 2. / beta * coth(gamma * Lc)
        Lt = 1. / gamma
    return Rc, Lt


# CNTMobility: compute the approximate CNT mobility 
#
# Input:
#   L:  gate length [m]
#   d:  CNT diameter [m]
#
# Output:
#   mu: CNT mobility [m^2/V/s]
#
# Comment:
def CNTMobility(L, d):
    dnm = d * 1e9
    # for Lg=5-50nm
    #     mu0 = 0.135;
    #     lmu = 48.6e-9;
    #     cmu = 1.13;
    # for Lg=50-100nm
    mu0 = 0.135
    lmu = 66.2e-9
    cmu = 1.5

    mu = mu0 * L / (lmu + L) * (dnm ** cmu)
    return mu


# CapTopGate: gate oxide capacitance of a top-gated struture
#
# Input:
#   s:      spacing of CNTs
#   d:      CNT diameter [m]
#   tox:    gate oxide thickness [m]
#   kox:    dielectric constant of gate oxide
#   W:      device width [m]
#   scmod:  charge screening effect switch: 0-off, 1-on
#
# Output:
#   Cgc:    gate oxide capacitance per longitudinal unit length [F/m]
#
# Comment:
#   Eq (13) in "Modeling and Analysis of Planar-Gate Electrostatic
#   Capacitance of 1-D FET With Multiple Cylindrical Conducting Channels"
#   Single diameter and uniform spacing.
#   substrate is assumed to be SiO2
def CapTopGate(s, d, tox, kox, W, scmod):
    #% With screening effect
    N = ceil(W / s)
    eps0 = 8.85e-12
    ksub = 3.9
    r = d / 2
    h = tox + r
    N1 = np.pi * eps0 * kox
    C1 = (kox - ksub) / (kox + ksub)# denoted lambda1 in the paper
    Cgc_S = 2 * N1 / (np.arccosh((tox + r) / r) + C1 * np.log((h + 2 * r) / 3. / r))# Cgs_inf
    if scmod == 0:
        Cgc = Cgc_S * N
    else:
        if N == 1:
            Cgc = Cgc_S
        else:
            D1 = np.log((s ** 2 + 2 * (h - r) * (h + np.sqrt(h ** 2 - r ** 2))) / (s ** 2 + 2 * (h - r) * (h - np.sqrt(h ** 2 - r ** 2))))        # component of Cgc_sr
            D2 = C1 * np.log(((h + 2 * r) ** 2 + s ** 2) / (9 * r ** 2 + s ** 2)) * np.tanh((h + r) / (s - 2 * r))        # component of Cgc_sr
            Cgc_sr = 4 * N1 / (D1 + D2)
            Cgc_E = Cgc_S * Cgc_sr / (Cgc_S + Cgc_sr)
            Cgc_M = 2 * Cgc_E - Cgc_S
            Cgc = 2 * Cgc_E + (N - 2) * Cgc_M
    return Cgc


# CapTopGate: gate oxide capacitance of a top-gated struture
#
# Input:
#   s:      spacing of CNTs
#   d:      CNT diameter [m]
#   tox:    gate oxide thickness [m]
#   kox:    dielectric constant of gate oxide
#   W:      device width [m]
#   scmod:  charge screening effect switch: 0-off, 1-on
#
# Output:
#   Cgc:    gate oxide capacitance per longitudinal unit length [F/m]
#
# Comment:
#   Eq (13) in "Modeling and Analysis of Planar-Gate Electrostatic
#   Capacitance of 1-D FET With Multiple Cylindrical Conducting Channels"
#   Single diameter and uniform spacing.
#   substrate is assumed to be SiO2
def CapTopGate(s, d, tox, kox, W, scmod):
    #% With screening effect
    N = ceil(W / s)
    eps0 = 8.85e-12
    ksub = 3.9
    r = d / 2
    h = tox + r
    N1 = np.pi * eps0 * kox
    C1 = (kox - ksub) / (kox + ksub)# denoted lambda1 in the paper
    Cgc_S = 2 * N1 / (np.arccosh((tox + r) / r) + C1 * np.log((h + 2 * r) / 3. / r))# Cgs_inf
    if scmod == 0:
        Cgc = Cgc_S * N
    else:
        if N == 1:
            Cgc = Cgc_S
        else:
            D1 = np.log((s ** 2 + 2 * (h - r) * (h + np.sqrt(h ** 2 - r ** 2))) / (s ** 2 + 2 * (h - r) * (h - np.sqrt(h ** 2 - r ** 2))))        # component of Cgc_sr
            D2 = C1 * np.log(((h + 2 * r) ** 2 + s ** 2) / (9 * r ** 2 + s ** 2)) * np.tanh((h + r) / (s - 2 * r))        # component of Cgc_sr
            Cgc_sr = 4 * N1 / (D1 + D2)
            Cgc_E = Cgc_S * Cgc_sr / (Cgc_S + Cgc_sr)
            Cgc_M = 2 * Cgc_E - Cgc_S
            Cgc = 2 * Cgc_E + (N - 2) * Cgc_M
            
    return Cgc


# CapGAA: Capacitance of a gate-all-around cylindrical structure
#
# Input:
#   tox:    gate oxide thickness
#   d:      CNT diameter
#   eox:    dielectric constact of gate oxide
#
# Output:
#   C:      gate oxide capacitance [F/m]
#
# Comment:
def CapGAA(d, tox, eox):
    C = 2 * np.pi * 8.85e-12 * eox / np.log((2 * tox + d) / d)
    return C

# CapFringe: CNT fringe capacitance 
# Input:
#   Lsd:    extension length/spacer width
#   tox:    gate oxide thickness
#   d:      CNT diameter
#   s:      pitch of CNTs
#   W:      device width
#   kspa:   dielectric constant of spacer
#   scmod:  screening mode:
#           0: no charge screening
#           1: with charge screening
# Output:
#   C:      single-sided fringe capacitance [F]
#   Cof_e:  fringe capacitance of edge CNTs
#   Cof_m:  fringe capacitance of middle CNTs
# Comment:
#   Ref: Eq. (17) in "Modeling and Analysis of Planar-Gate Electrostatic
#   Capacitance of 1-D FET With Multiple Cylindrical Conducting Channels"

def CapFringe(Lsd, tox, d, s, W, kspa, scmod):
    eps0 = 8.85e-12
    tao1 = 2.5
    tao2 = 2
    sr = 0.5

    N = round(W / s)# number of CNTs
    h = tox + d / 2
    A = sr * 2 * np.pi * kspa * eps0 * Lsd
    B = np.arccosh(2 * np.sqrt((h) ** 2 + (0.28 * Lsd) ** 2) / d)
    if N == 1 or scmod == 0:
        C = A / B * N
        Cof_e = C
        Cof_m = 0
    elif N == 2:
        Cof_e = A / (np.log(np.sqrt((2 * h) ** 2 + (0.56 * Lsd) ** 2 + s ** 2) / s) + B)
        C = 2 * Cof_e
        Cof_m = 0
    else:
        eta1 = np.exp((np.sqrt(N ** 2 - 2 * N) + N - 2) / tao1 / N)
        alpha = np.exp((N - 3) / tao2 / N)
        Cof_e = A / (np.log(np.sqrt((2 * h) ** 2 + (0.56 * Lsd) ** 2 + s ** 2) / s) + B)
        Cof_m = 2 * alpha / eta1 * Cof_e + (1 - 2 * alpha / eta1) * A / B
        C = 2 * Cof_e + (N - 2) * Cof_m
    return C, Cof_e, Cof_m
    
    
# CapC2C: single-sided parasitic capacitance between gate to contact
# Input:
#   Lsd:    extension length/spacer width
#   Lg:     gate length
#   Hg:     gate height
#   W:      device width
#   kspa:   dielectric constant of spacer
# Output:
#   C:      single-sided gate-to-contact parasitic capacitance [F]
# Comment:
#   Ref: Eq. (19) in "Modeling and Analysis of Planar-Gate Electrostatic
#   Capacitance of 1-D FET With Multiple Cylindrical Conducting Channels"
def CapC2C(Lsd, Lg, Hg, W, kspa):
    eps0 = 8.85e-12
    alpha = 1# 0.7 for Hg = Hc; assume 1 for Hc >> Hg
    tao = np.exp(2 - 2 * np.sqrt(1 + 2 * (Hg + Lg) / Lsd))
    C = kspa * eps0 * Hg / Lsd + alpha * np.pi * kspa * eps0 / np.log(2 * np.pi * (Lsd + Lg) / (2 * Lg + tao * Hg))
    C = C * W
    return C





#%%% Stanford Carbon Nanotube Field-Effect Transistor (SCNFET) Model %%%%

#%%%%%%%%%%%%%%%%%%%%
# Revision Log
#%%%%%%%%%%%%%%%%%%%%
# Ver 1.0.0     06/02/2014  by Chi-Shuen (Vince) Lee
#       - Create the first version of SCNFET model
#   - The source-to-drain-tunneling is based on Ec = a*np.exp(x/lam)+b
#%%%%%%%%%%%%%%%%%%%%

# Input:
#       vd0,vg0,vs0:    drain, gate, source voltage
#       Lg:     gate length
#       Lc:     contact length
#       Lsd:    source/drain extension length
#       W:      device width
#       tox:    gate oxide thickness
#       kox:    gate oxide dielectric constant
#       geo:    device geometry
#               - 1: gate-all-around
#               - 2: top-gate with charge screening effect
#               - 3: top-gate without charge screening effect
#       Hg:     metal gate height
#       kspa:   dielectric constant of the source/drain spacer
#       dcnt:   CNT diameter
#       cspa:   center-center distance between the adjacent CNTs
#       Efsd:   Fermi-level at the source/drain minus to the Ec [eV]
#               determined by the doping density
#       Vfb:    flat-band voltage [V]
#       Temp:   temperature in Celsius
#       rcmod:  Contact mode:
#               - 0: user-defined series resistance
#               - 1: transmission line model with diameter dependence
#               - 2: transmission line model without diameter dependence
#       rs0:    user-defined series resistance (Ohm per CNT, one-sided)
#       SDTmod: - 0: SDT off
#               - 1: SDT on
#               - 2: SDT on, with a lower limit of surface potential
#                    no CB-VB-CB tunneling
#       BTBTmod:- 0: without BTBT
#               - 1: with BTBT
# Output:
#       Id:     total drain current [A]
#       Ivs:    thermionic emission current [A], given by the VS model
#       Isdt:   source-to-drain tunneling current [A]
#       Ibtbt:  band-to-band tunneling current [A]
#       Qs,Qd,Qg: terminal charge without the parasitic contribution [C]
#       Cinv:   inversion gate capacitance per unit length [F/m]
#       Cp:     parasitic capacitance [F]
# Comment:
#       All the inputs and outputs follows the MKS system of units
#       The output current is for one device, not for one CNT
#       Series resistance is connected externally and solved iteratively
def vscnfet_1_0_1_woi(vd0, vg0, vs0, Lg, Lc, Lsd, W, tox, kox, geo, Hg, kspa, dcnt, cspa, Efsd, Vfb, Temp, rcmod, rs0, SDTmod, BTBTmod):
    # SDTProb: SDT probability; Ec = a*exp(x/lambda)+b
    # Et == Ec(x=0); El == Ec(x=L)
    def SDTProb(Et, El, Ei, L, Eg, _lambda):
        a = -(Et - El) / (np.exp(L / _lambda) - 1)
        b = Et - a
        x0 = 1 - 2 * (a + b - Ei) / Eg
        x0[Et > Ei + Eg] = -1
        x1 = 1
        zeta = 2 * (b - Ei) / Eg - 1
        y = (SDTProbInt(x1, zeta) - SDTProbInt(x0, zeta)) * _lambda
        return y
    
    # SDTProbInt: integrate sqrt(1-x^2)/(x+zeta)   
    def SDTProbInt(x, zeta):
        rb = np.sqrt(zeta ** 2 - 1 + 0j) * np.sqrt(1 - x ** 2 + 0j) / (zeta * x + 1)
        z = zeta * x + 1
        theta = np.arctan(rb)
        theta[z == 0] = -np.pi / 2
        theta[z > 0] = theta[z > 0] - np.pi
        y = np.sqrt(zeta ** 2 - 1 + 0j) * theta + zeta * np.arcsin(x) + np.sqrt(1 - x ** 2 + 0j)
        return y
    
    # source-to-drain tunneling function
    def SDT(Eg, Efsd, psys, Lg, Lof, vds, lam, vt, N_tun):
        h = 6.626e-34
        h_bar = h / 2 / np.pi
        vF = 1e6
        q = 1.6e-19
        
        Lh = Lg / 2 + Lof
        
        # top of the barrier; to avoid Et < -Efsd
        Et0 = Eg / 2 - psys
        aEt = 0.05
        Et = (Et0 + Efsd) + aEt * np.log(1 + np.exp(-(Et0 + Efsd) / aEt)) - Efsd
        
        Eitop = min(Et, 0.4)
        dE = (Eitop + Efsd) / N_tun
        Ei = np.arange(-Efsd + dE / 2, Eitop, dE)
        
        prefac = Eg * q / (h_bar * vF)
        tbs = SDTProb(Et, -Efsd, Ei, Lh, Eg, lam)
        tbd = SDTProb(Et, -Efsd - vds, Ei, Lh, Eg, lam)
        Ti = np.exp(-prefac * (tbs + tbd))
        Efs = 0
        Efd = -vds
        igrid = Ti * (1. / (1 + np.exp((Ei - Efs) / vt)) - 1. / (1 + np.exp(((Ei - Efd)) / vt)))
        isdt = 4 * q ** 2 / h * np.sum(igrid) * dE
        return isdt

    
    
    
    
    # constants
    m0 = 9.11e-31# electron mass
    kB = 1.38e-23# Boltzmann constant
    q = 1.6e-19# electron charge
    h = 6.626e-34# Planck constant
    h_bar = h / 2 / np.pi# reduced Planck constant
    acc = 0.142e-9# carbon-carbon distance
    Vp = 3# CNT tight binding parameter
    vF = 1e6# Fermi velocity
    vt = (Temp + 273.15) * kB / q# thermal voltage

    # Bias
    vds0 = abs(vd0 - vs0)
    vgs0 = vg0 - min(vs0, vd0)

    # Virtual source related internal parameters
    Ncnt = round(W / cspa)# number of CNTs in the device
    Eg = 2 * acc * Vp / dcnt# band gap [eV]
    kch = 1# CNT dielectric constant
    Lof_sce = tox / 3# SCE fitting parameter
    cqa = 0.64 * 1e-9# CNT quantum capacitance param 1
    cqb = 0.1 * 1e-9# CNT quantum capacitance param 2
    vB0 = 4.1e5# ballistic velocity for CNT = 1.2 nm
    d0 = 1.2e-9# reference CNT diameter
    mfpv = 220e-9# carrier mean free path for velocity calculation
    vtha = 0.0# fitting parameter to match VS current and DSDT

    #%%% VS parameters %%%%
    # Inversion capacitance
    if geo == 1:    # GAA
        cox = CapGAA(dcnt, tox, kox) * Ncnt
    elif geo == 2:    # top-gate with screening
        cox = CapTopGate(cspa, dcnt, tox, kox, W, 1)
    elif geo == 3:    # top-gate w/o screening
        cox = CapTopGate(cspa, dcnt, tox, kox, W, 0)
    else:
        cox = CapGAA(dcnt, tox, kox) * Ncnt
    
    cqeff = (cqa * np.sqrt(Eg) + cqb) * Ncnt    # effecitve quantum capacitance
    Cinv = cox * cqeff / (cox + cqeff)
    mu = CNTMobility(Lg, dcnt)    # Mobility
    # SCE parameters
    nsd = CNTCarrierApprox(dcnt, Efsd + Eg / 2, Temp)    # source/drain doping density
    lam = Scalelength_GAA(kch, kox, dcnt, tox)    # scale length
    eta = np.exp(-(Lg + Lof_sce * 2) / lam / 2)
    n0 = 1 / (1 - 2 * eta)    # SS factor at Vd = 0
    dibl = eta    # drain-induced barrier lowering
    dvth = -2 * (Efsd + Eg / 2) * eta    # vt roll-off
    vB = vB0 * np.sqrt(dcnt / d0)    # ballistic velocity
    vxo = vB * mfpv / (mfpv + Lg)    # virtual source velocity
    # Series resistance
    Rc = RcCNT(Lc, dcnt, rcmod)
    Rext = RextCNT(Lsd, nsd, dcnt)
    if rcmod == 0:
        rs = rs0 / Ncnt
    else:
        rs = (Rc + Rext) / Ncnt
    
    vth0 = Eg / 2 + dvth + Vfb + vtha        # Threshold voltage
    alpha = 3.5        # VS fitting parameter
    beta = 1.8        # VS fitting parameter
    nd = 0        # drain bias dependent SS factor

    #%%% Tunneling parameter %%%%
    Lof_sdt = (0.0263 * kspa + 0.056) * tox
    #     Lof_sdt = tox/2;    % SDT fitting parameter, gate length offset
    phi_on = 0.05        # SDT fitting parameter, Vt adjustment
    N_tun = 100        # resolution of energy integral
    ScatFac = mfpv / (mfpv + Lg)
    lam_btb = 2.5e-9 + 1.1e-9 / 12 * (kspa - 4)        # BTBT fitting parameter


    #%%% Calculate current %%%%
    vgsi = vgs0
    vdsi = vds0
    vgs = vgs0
    vds = vds0

    #%%% virtual source current %%%%
    vth = vth0 - dibl * vds
    Ff = 1 / (1 + np.exp((vgs - (vth - alpha * vt / 2)) / alpha / vt))
    nss = n0 + nd * vds
    Qxo = Cinv * nss * vt * np.log(1 + np.exp((vgs - (vth - alpha * vt * Ff)) / nss / vt))
    vx = (Ff + (1 - Ff) / (1 + rs * Cinv * (1 + 2 * dibl) * vxo)) * vxo
    vdsats = (rs + rs) * Qxo * vx + vx * Lg / mu
    vdsat = vdsats * (1 - Ff) + vt * Ff
    Fs = vds / vdsat / (1 + (vds / vdsat) ** beta) ** (1 / beta)
    Ivs = Qxo * vx * Fs

    #%%% source-to-drain tunneling %%%%
    if SDTmod != 0:
        # surface potential
        if SDTmod == 1:
            if Qxo == 0:
                psys = (vgs - (vth - alpha * vt * Ff)) / nss + Eg / 2 - phi_on
            else:
                psys = np.log(Qxo / (Cinv * nss * vt)) * vt + Eg / 2 - phi_on
        else:
            # psys is clamped at occurrence of CB-VB-CB tunneling
            Qmin = np.exp((Efsd - Eg + phi_on) / vt)
            psys = np.log(Qxo / (Cinv * nss * vt) + Qmin) * vt + Eg / 2 - phi_on
        
        Isdt = SDT(Eg, Efsd, psys, Lg, Lof_sdt, vds, lam, vt, N_tun)
        Isdt = Isdt * ScatFac * Ncnt
    else:
        Isdt = 0
    

    #%%% band-to-band tunneling %%%%
    if vds <= Eg or BTBTmod == 0:
        Ibtbt = 0
    else:
        Ew = vds - Eg
        dE = Ew / N_tun
        Ei = mslice[dE / 2:dE:Ew]
        prefac = Eg * q / (h_bar * vF)
        zeta = -1 - 2 * Ei / Eg
        tb = -lam_btb * np.pi * (zeta + np.sqrt(zeta ** 2 - 1))
        Ti = np.exp(-prefac * tb)
        Efs = Efsd + vds
        Efd = Efsd
        igrid = Ti * (1. / (1 + np.exp((Ei - Efs) / vt)) - 1. / (1 + np.exp(((Ei - Efd)) / vt)))
        Ibtbt = 4 * q ** 2 / h * np.sum(igrid) * dE * ScatFac * Ncnt
    

    Id = Ivs + Isdt + Ibtbt

    #%%% Charge model %%%%
    # Internal Parameters
    avt = alpha * vt
    nss = n0 + nd * vdsi                        # SS factor
    vth = vth0 - dibl * vdsi                        # threshold voltage
    Ff = 1 / (1 + np.exp((vgsi - (vth - avt / 2)) / avt))
    Qcha = Cinv * nss * vt * np.log(1 + np.exp((vgsi - (vth - avt * Ff)) / nss / vt))

    # Charge correction due to 1-D quantum capacitance
    alphab = 3.5
    bvt = alphab * vt
    nb = 1.5
    vthb = vth + 0.2 * Eg + 0.16
    Ffb = 1 / (1 + np.exp((vgsi - (vthb - bvt / 2)) / bvt))
    cqinf = 8 * q / (3 * acc * np.pi * Vp) * Ncnt                        # cqinf = 3.2e-10 [F/m]
    cinvb = cox * cqinf / (cox + cqinf)
    Qchb = (Cinv - cinvb) * nb * vt * np.log(1 + np.exp((vgsi - (vthb - bvt * Ffb)) / nb / vt))
    Qch = -Lg * (Qcha - Qchb)

    # Charge version vdsat
    vgt = -Qch / Cinv / Lg
    Vdsatq = np.sqrt(Ff * avt ** 2 + vgt ** 2)
    Fsatq = (vdsi / Vdsatq) / ((1 + (vdsi / Vdsatq) ** beta) ** (1 / beta))

    # Drift-Diffusion, Tsividis, 2nd Ed., Eq. (7.4.19) and (7.4.20)
    eta = 1 - Fsatq
    QsDD = Qch * (6 + 12 * eta + 8 * eta ** 2 + 4 * eta ** 3) / (15 * (1 + eta) ** 2)
    QdDD = Qch * (4 + 8 * eta + 12 * eta ** 2 + 6 * eta ** 3) / (15 * (1 + eta) ** 2)

    # Quasi-Ballistic transport, assume parabolic potential profile
    zeta = 1.0                        # fitting parameter
    meff = 2 / 9 * Eg / q * (h_bar / acc / Vp) ** 2 / m0                        # CNT effective mass
    if vdsi == 0:
        QsB = Qch / 2
        QdB = Qch / 2
    else:
        kqb = 2 * q * vdsi * zeta / (meff * m0 * vxo ** 2)
        QsB = Qch * (np.arcsinh(np.sqrt(kqb)) / np.sqrt(kqb) - (np.sqrt(kqb + 1) - 1) / kqb)
        QdB = Qch * ((np.sqrt(kqb + 1) - 1) / kqb)
    

    Fsatq2 = Fsatq ** 2
    Qs = QsDD * (1 - Fsatq2) + QsB * Fsatq2
    Qd = QdDD * (1 - Fsatq2) + QdB * Fsatq2
    Qg = -(Qs + Qd)

    #%%% Parasitic Capacitance %%%%
    if geo == 2:
        scmod = 1                                # fringe capacitance w/ charge screening
    else:
        scmod = 0                                # fringe capacitance w/o charge screening
    
    Cf = CapFringe(Lsd, tox, dcnt, cspa, W, kspa, scmod)                                # fringe capacitance
    Cg2c = CapC2C(Lsd, Lg, Hg, W, kspa)                                # gate-to-contact parasitic capacitance
    Cp = 2 * (Cf + Cg2c)
    return Id, Ivs, Isdt, Ibtbt, Qs, Qd, Qg, Cinv, Cp

                                    # source-to-drain tunneling function



#%%% Stanford Virtual-Source Carbon Nanotube Field-Effect Transistor (VSCNFET) Model %%%%

#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#Copyright @ 2014 Stanford University (Stanford)
#The terms under which the software and associated documentation (the Software) is provided are as the following:
#The Software is provided "as is", without warranty of any kind, express or implied, including but not limited to the warranties of merchantability, fitness for a particular purpose and noninfringement. In no event shall the authors or copyright holders be liable for any claim, damages or other liability, whether in an action of contract, tort or otherwise, arising from, out of or in connection with the Software or the use or other dealings in the Software.
#Stanford grants, free of charge, to any users the right to modify, copy, and redistribute the Software, both within the user's organization and externally, subject to the following restrictions:
#1. The users agree not to charge for the Stanford code itself but may charge for additions, extensions, or support.
#2. In any product based on the Software, the users agree to acknowledge the Stanford Nanoelectronics Research Group of Prof. H.-S. Philip Wong that developed the software and cite the relevant publications that form the basis of the Software. This acknowledgment shall appear in the product documentation.
#3. The users agree to obey all U.S. Government restrictions governing redistribution or export of the software.
#4. The users agree to reproduce any copyright notice which appears on the software on any copy or modification of such made available to others.

# Agreed to by 
# H.-S. Philip Wong, Stanford University
# May 4, 2014
#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

#%%%%%%%%%%%%%%%%%%%%
# Revision Log
#%%%%%%%%%%%%%%%%%%%%
# Ver 1.0.1     04/01/2015  by Chi-Shuen (Vince) Lee
# Modify the charge model to avoid differentiation problem at Vds = 0
#%%%%%%%%%%%%%%%%%%%%
# Ver 1.0.0     06/02/2014  by Chi-Shuen (Vince) Lee
#       - Implemented Version 1.0.0 VSCNFET model
#%%%%%%%%%%%%%%%%%%%%

# Input:
#       vd0:    drain voltage
#       vg0:    gate voltage
#       vs0:    source voltage
#       Lg:     gate length
#       Lc:     contact length
#       Lsd:    source/drain extension length
#       W:      device width
#       tox:    gate oxide thickness
#       kox:    gate oxide dielectric constant
#       geo:    device geometry
#               - 1: gate-all-around
#               - 2: top-gate with charge screening effect
#               - 3: top-gate without charge screening effect
#       Hg:     metal gate height
#       kspa:   dielectric constant of the source/drain spacer
#       dcnt:   CNT diameter
#       s:      center-center distance between the adjacent CNTs
#       Efsd:   Fermi-level at the source/drain minus to the Ec [eV]
#               determined by the doping density
#       Vfb:    flat-band voltage [V]
#       Temp:   temperature in Celsius
#       rcmod:  Contact mode:
#               - 0: user-defined series resistance
#               - 1: transmission line model with diameter dependence
#               - 2: transmission line model without diameter dependence
#       rs0:    user-defined series resistance (Ohm per CNT, one-sided)
#       SDTmod: - 0: SDT off
#               - 1: SDT on
#               - 2: SDT on, with a lower limit of surface potential
#                    no CB-VB-CB tunneling
#       BTBTmod:- 0: without BTBT
#               - 1: with BTBT
# Output:
#       Id:     total drain current [A]
#       Ivs:    thermionic emission current [A], given by the VS model
#       Isdt:   source-to-drain tunneling current [A]
#       Ibtbt:  band-to-band tunneling current [A]
#       Qs,Qd,Qg: terminal charge without the parasitic contribution [C]
#       Cinv:   inversion gate capacitance per unit length [F/m]
#       Cp:     one-sided parasitic capacitance [F]
# Comment:
#       All the inputs and outputs follows the MKS system of units
#       The output current is for one device, not for one CNT
#       Series resistance is connected externally and solved iteratively
def vscnfet_1_0_1(vd0, vg0, vs0, Lg, Lc, Lsd, W, tox, kox, geo, Hg, kspa, dcnt, s, Efsd, Vfb, Temp, rcmod, rs0, SDTmod, BTBTmod):
    # myfun: iterative solver for internal node voltages
    def myfun(rs, vs, vds, vgs, Lg, Cinv, mu, n0, dibl, vth0, alpha, beta, vxo, nd, vt, Eg, phi_on, Efsd, lam, Lof_sdt, Ncnt, ScatFac, N_tun, lam_btb, SDTmod, BTBTmod):
        vdsi = vds - 2 * vs
        vgsi = vgs - vs
        Id = IdInternal(vdsi, vgsi, Lg, Cinv, mu, n0, dibl, vth0, alpha, beta, vxo, nd, vt, Eg, phi_on, Efsd, lam, Lof_sdt, Ncnt, ScatFac, N_tun, lam_btb, SDTmod, BTBTmod)
        Id = real(Id)
        f = Id * rs - vs
        return f

    # source-to-drain tunneling function
    def SDT(Eg, Efsd, psys, Lg, Lof, vds, lam, vt, N_tun):
        h = 6.626e-34
        h_bar = h / 2 / pi
        vF = 1e6
        q = 1.6e-19

        Lh = Lg / 2 + Lof

        # top of the barrier; to avoid Et < -Efsd
        Et0 = Eg / 2 - psys
        aEt = 0.05
        Et = (Et0 + Efsd) + aEt * np.log(1 + np.exp(-(Et0 + Efsd) / aEt)) - Efsd
        Etx = Et + Efsd

        Eitop = min(Et, 0.4)
        #     Eitop   = Et;
        dE = (Eitop + Efsd) / N_tun
        Ei = mslice[-Efsd + dE / 2:dE:Eitop]

        prefac = Eg * q / (h_bar * vF)
        tbs = SDTProb(Et, -Efsd, Ei, Lh, Eg, lam)
        tbd = SDTProb(Et, -Efsd - vds, Ei, Lh, Eg, lam)
        Ti = np.exp(-prefac * (tbs + tbd))
        Efs = 0
        Efd = -vds
        igrid = Ti * (1. / (1 + np.exp((Ei - Efs) / vt)) - 1. / (1 + np.exp(((Ei - Efd)) / vt)))
        isdt = 4 * q ** 2 / h * np.sum(igrid) * dE
        return isdt

    # SDTProb: SDT probability; Ec = a*np.exp(x/lambda)+b
    # Et == Ec(x=0); El == Ec(x=L)
    def SDTProb(Et, El, Ei, L, Eg, _lambda):
        a = -(Et - El) / (np.exp(L / _lambda) - 1)
        b = Et - a
        x0 = 1 - 2 * (a + b - Ei) / Eg
        x0[Et > Ei + Eg] = -1
        x1 = 1
        zeta = 2 * (b - Ei) / Eg - 1
        y = (SDTProbInt(x1, zeta) - SDTProbInt(x0, zeta)) * _lambda
        return y

    # SDTProbInt: integrate sqrt(1-x^2)/(x+zeta)
    def SDTProbInt(x, zeta):
        rb = sqrt(zeta ** 2 - 1 + 0j) * np.sqrt(1 - x ** 2 + 0j) / (zeta * x + 1)
        z = zeta * x + 1
        theta = np.arctan(rb)
        theta[z == 0] = -np.pi / 2
        theta[z > 0] = theta(z > 0) - np.pi
        y = np.sqrt(zeta ** 2 - 1 + 0j) * theta + zeta * np.arcsin(x) + np.sqrt(1 - x ** 2 + 0j)
        return y
    
    
    
    # constants
    m0 = 9.11e-31# electron mass
    kB = 1.38e-23# Boltzmann constant
    q = 1.6e-19# electron charge
    h = 6.626e-34# Planck constant
    h_bar = h / 2 / np.pi# reduced Planck constant
    acc = 0.142e-9# carbon-carbon distance
    Vp = 3# CNT tight binding parameter
    vt = (Temp + 273.15) * kB / q# thermal voltage

    # Bias
    vds0 = abs(vd0 - vs0)
    vgs0 = vg0 - min(vs0, vd0)

    # Virtual source related internal parameters
    Ncnt = round(W / s)# number of CNTs in the device
    Eg = 2 * acc * Vp / dcnt# band gap [eV]
    kch = 1# CNT dielectric constant
    Lof_sce = tox / 3# SCE fitting parameter
    cqa = 0.64 * 1e-9# CNT quantum capacitance param 1
    cqb = 0.1 * 1e-9# CNT quantum capacitance param 2
    vB0 = 4.1e5# ballistic velocity for CNT = 1.2 nm
    d0 = 1.2e-9# reference CNT diameter
    mfpv = 440e-9# carrier mean free path for velocity calculation
    vtha = 0.0# fitting parameter to match VS current and DSDT

    #%%% VS parameters %%%%
    # Inversion capacitance
    if geo == 1:    # GAA
        cox = CapGAA(dcnt, tox, kox) * Ncnt
    elif geo == 2:    # top-gate with screening
        cox = CapTopGate(s, dcnt, tox, kox, W, 1)
    elif geo == 3:    # top-gate w/o screening
        cox = CapTopGate(s, dcnt, tox, kox, W, 0)
    else:
        cox = CapGAA(dcnt, tox, kox) * Ncnt
    
    cqeff = (cqa * np.sqrt(Eg) + cqb) * Ncnt    # effecitve quantum capacitance
    Cinv = cox * cqeff / (cox + cqeff)
    mu = CNTMobility(Lg, dcnt)    # Mobility
    # SCE parameters
    nsd = CNTCarrierApprox(dcnt, Efsd + Eg / 2, Temp)    # source/drain doping density
    lam = Scalelength_GAA(kch, kox, dcnt, tox)    # scale length
    eta = np.exp(-(Lg + Lof_sce * 2) / lam / 2)
    n0 = 1 / (1 - 2 * eta)    # SS factor at Vd = 0
    dibl = eta    # drain-induced barrier lowering
    dvth = -2 * (Efsd + Eg / 2) * eta    # vt roll-off
    vB = vB0 * np.sqrt(dcnt / d0)    # ballistic velocity
    vxo = vB * mfpv / (mfpv + Lg * 2)    # virtual source velocity
    # Series resistance
    Rc = RcCNT(Lc, dcnt, rcmod)
    Rext = RextCNT(Lsd, nsd, dcnt)
    if rcmod == 0:
        rs = rs0 / Ncnt
    else:
        rs = (Rc + Rext) / Ncnt
    
    vth0 = Eg / 2 + dvth + Vfb + vtha        # Threshold voltage
    alpha = 3.5        # VS fitting parameter
    beta = 1.8        # VS fitting parameter
    nd = 0        # drain bias dependent SS factor

    #%%% Tunneling parameter %%%%
    Lof_sdt = (0.0263 * kspa + 0.056) * tox        # SDT fitting parameter, gate length offset
    #     Lof_sdt = tox/2;
    phi_on = 0.05        # SDT fitting parameter, Vt adjustment
    N_tun = 100        # resolution of energy integral
    ScatFac = mfpv / (mfpv + 2 * Lg)
    lam_btb = (0.092 * kspa + 2.13) * 1e-9


    #%%% Calculate current %%%%
    # solve internal source/drain voltages 
    [vsi, FVAL, EXITFLAG] = fzero(lambda vs: myfun(rs, vs, vds0, vgs0, Lg, Cinv, mu, n0, dibl, vth0, alpha, beta, vxo, nd, vt, Eg, phi_on, Efsd, lam, Lof_sdt, Ncnt, ScatFac, N_tun, lam_btb, SDTmod, BTBTmod), 0)
    vgsi = vgs0 - vsi
    vdsi = vds0 - 2 * vsi

    [Id, Ivs, Isdt, Ibtbt] = IdInternal(vdsi, vgsi, Lg, Cinv, mu, n0, dibl, vth0, alpha, beta, vxo, nd, vt, Eg, phi_on, Efsd, lam, Lof_sdt, Ncnt, ScatFac, N_tun, lam_btb, SDTmod, BTBTmod)


    #%%% Charge model %%%%
    # Internal Parameters
    avt = alpha * vt
    nss = n0 + nd * vdsi        # SS factor
    vth = vth0 - dibl * vdsi        # threshold voltage
    Ff = 1 / (1 + np.exp((vgsi - (vth - avt / 2)) / avt))
    Qcha = Cinv * nss * vt * np.log(1 + np.exp((vgsi - (vth - avt * Ff)) / nss / vt))

    # Charge correction due to 1-D quantum capacitance
    alphab = 3.5
    bvt = alphab * vt
    nb = 1.5
    vthb = vth + 0.2 * Eg + 0.13
    Ffb = 1 / (1 + np.exp((vgsi - (vthb - bvt / 2)) / bvt))
    cqinf = 8 * q / (3 * acc * np.pi * Vp) * Ncnt        # cqinf = 3.2e-10 [F/m]
    cinvb = cox * cqinf / (cox + cqinf)
    Qchb = (Cinv - cinvb) * nb * vt * np.log(1 + np.exp((vgsi - (vthb - bvt * Ffb)) / nb / vt))
    Qch = -Lg * (Qcha - Qchb)

    # Charge version vdsat
    vgt = Qcha / Cinv
    Vdsatq = np.sqrt(Ff * avt ** 2 + vgt ** 2)
    Fsatq = (vdsi / Vdsatq) / ((1 + (vdsi / Vdsatq) ** beta) ** (1 / beta))

    # Drift-Diffusion, Tsividis, 2nd Ed., Eq. (7.4.19) and (7.4.20)
    eta = 1 - Fsatq
    QsDD = Qch * (6 + 12 * eta + 8 * eta ** 2 + 4 * eta ** 3) / (15 * (1 + eta) ** 2)
    QdDD = Qch * (4 + 8 * eta + 12 * eta ** 2 + 6 * eta ** 3) / (15 * (1 + eta) ** 2)

    # Quasi-Ballistic transport, assume parabolic potential profile
    zeta = 0.1        # fitting parameter
    meff = 2 / 9 * Eg / q * (h_bar / acc / Vp) ** 2 / m0        # CNT effective mass
    kqb = 2 * q * vdsi * zeta / (meff * m0 * vxo ** 2)
    SMALL_VAL = 1e-20
    tol = SMALL_VAL * (vxo * vxo * meff / 2 / q / zeta)
    if vdsi < tol:
        QsB = Qch * (0.5 - kqb / 24 + kqb ** 2 / 80)
        QdB = Qch * (0.5 - kqb / 8 + kqb ** 2 / 16)
    else:
        QsB = Qch * (np.arcsinh(np.sqrt(kqb)) / np.sqrt(kqb) - (np.sqrt(kqb + 1) - 1) / kqb)
        QdB = Qch * ((np.sqrt(kqb + 1) - 1) / kqb)
    

    Fsatq2 = Fsatq ** 2
    Qs = QsDD * (1 - Fsatq2) + QsB * Fsatq2
    Qd = QdDD * (1 - Fsatq2) + QdB * Fsatq2
    Qg = -(Qs + Qd)

    #%%% Parasitic Capacitance %%%%
    if geo == 2:
        scmod = 1                # fringe capacitance w/ charge screening
    else:
        scmod = 0                # fringe capacitance w/o charge screening
    
    Cf = CapFringe(Lsd, tox, dcnt, s, W, kspa, scmod)                # fringe capacitance
    Cg2c = CapC2C(Lsd, Lg, Hg, W, kspa)                # gate-to-contact parasitic capacitance
    Cp = (Cf + Cg2c)
    return Id, Ivs, Isdt, Ibtbt, Qs, Qd, Qg, Cinv, Cp
    
    
























# Geometry and design parameters
Lg      = 11.7e-9
Lc      = 12.9e-9
Lext    = 3.2e-9
W       = 1e-6
tox     = 3e-9
kox     = 23
geo     = 1
Hg      = 20e-9
kspa    = 7.5
dcnt    = 1.2e-9
cspa    = 10e-9
Efsd    = 0.258
Vfb     = 0.015
Temp    = 25
rcmod   = 1
rs0     = 0
SDTmod  = 2
# SDTmod  = 0
BTBTmod = 1
Vdd     = 0.71

vg = np.arange(-Vdd, Vdd, 0.01)
vd_bias = [0.05, Vdd]
vs = 0

# for iid in range(len(vd_bias)):
#     for iig in range(len(vg)):
#         [Id(iig, iid), Ivs(iig, iid), Idsdt(iig, iid), Ibtbt(iig, iid), Qs(iig, iid), Qd(iig, iid), Qg(iig, iid)] = vscnfet_1_0_1(vd_bias(iid), vg(iig), vs, Lg, Lc, Lext, W, tox, kox, geo, Hg, kspa, dcnt, cspa, Efsd, Vfb, Temp, rcmod, rs0, SDTmod, BTBTmod)

# figure(1)
# semilogy(vg, Id, mstring('linewidth'), 4)
# set(gca, mstring('linew'), 4, mstring('fontsize'), 20)
# ylabel(mstring('(log) I_d (A/\\mum)'), mstring('fontsize'), 20)
# xlabel(mstring('V_g_s (V)'), mstring('fontsize'), 20)
# set(gca, mstring('ylim'), mcat([1e-9, 1e-2]), mstring('ytick'), logspace(-9, -2, 8))
# set(gca, mstring('xlim'), mcat([-inf, inf]))

# Lgs = [Lg]
Lgs = [10e-9, 20e-9, 30e-9, 40e-9, 50e-9]
Lgs = [20e-9]
Vgss = np.arange(-1,18,0.01)

for Lg in Lgs:
    Idss = []
    for Vgs in Vgss:
        # result = vscnfet_1_0_1(0.1, 0, vs, Lg, Lc, Lext, W, tox, kox, geo, Hg, kspa, dcnt, cspa, Efsd, Vfb, Temp, rcmod, rs0, SDTmod, BTBTmod)
        result = vscnfet_1_0_1_woi(0.1, Vgs, vs, Lg, Lc, Lext, W, tox, kox, geo, Hg, kspa, dcnt, cspa, Efsd, Vfb, Temp, rcmod, rs0, SDTmod, BTBTmod)
        Id1,Id2,Ivs1,Ivs2,Idsdt,Ibtbt,Qs,Qd,Qg = result
        Idss.append(result[0][1])
    
    # plt.plot(Vgss, np.real(Idss), label='Lg = {}'.format(Lg))


Idss = np.real(Idss)

# p = np.polyfit(Vgss, Idss, 5, w=np.real(Idss)**2)
# print(p)
# plt.plot(Vgss, np.polyval(p, Vgss))
# plt.plot(Vgss, np.polyval([0,0,0,0,p[4],p[5]], Vgss), label='Linear')
# plt.plot(Vgss, np.polyval([0,0,0,p[3],p[4],p[5]], Vgss), label='Quadratic')
# plt.plot(Vgss, np.polyval([0,0,p[2],p[3],p[4],p[5]], Vgss), label='Cubic')
# plt.plot(Vgss, np.polyval([0,p[1],p[2],p[3],p[4],p[5]], Vgss), label='Quartic')


# p = np.polyfit(Vgss, Idss**3, 1, w=Idss**3)
# plt.plot(Vgss, Idss)
# plt.plot(Vgss, np.cbrt(np.polyval(p, Vgss)))

# plt.plot(Vgss, Idss**2/max(Idss**2))
# plt.plot(Vgss, Idss**3/max(Idss**3))
# plt.plot(Vgss, Idss**4/max(Idss**4))
# plt.plot(Vgss, (np.exp(Idss)-1)/max(np.exp(Idss)-1))


# p = np.polyfit(Vgss, np.real(Idss), 5, w=np.real(Idss)**2)
# residuesLinear = np.polyval([0,0,0,0,p[4],p[5]], Vgss) - Idss
# plt.plot(Vgss, residuesLinear)

# plt.plot(Vgss, Idss)
# power = 0.9 - Vgss/100
# model = np.real(np.power(Vgss + 0j, power))
# plt.plot(Vgss, model*max(Idss)/max(model))

p = np.polyfit(Idss, Vgss, 3)
print(p)
# plt.plot(Idss, Vgss)
# plt.plot(Idss, np.polyval(p, Idss))
# plt.plot(Vgss, Idss)
# plt.plot(np.polyval(p, Idss), Idss)

# plt.plot(np.cbrt(Vgss), Idss)

plt.plot(Vgss, Idss)
model = np.maximum(0,np.log(np.abs(Vgss+5)))
model -= min(model)+0.3
# plt.plot(Vgss, model*max(Idss)/max(model))


print('Simulation complete')
plt.legend(loc='best')
plt.xlabel('Gate Voltage, $V_{GS}$ [V]')
plt.ylabel('Drain Current, $I_D$, [A]')
plt.show()



