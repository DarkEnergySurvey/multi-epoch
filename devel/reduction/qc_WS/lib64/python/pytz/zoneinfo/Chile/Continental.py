'''tzinfo timezone information for Chile/Continental.'''
from pytz.tzinfo import DstTzInfo
from pytz.tzinfo import memorized_datetime as d
from pytz.tzinfo import memorized_ttinfo as i

class Continental(DstTzInfo):
    '''Chile/Continental timezone definition. See datetime.tzinfo for details'''

    _zone = 'Chile/Continental'

    _utc_transition_times = [
d(1,1,1,0,0,0),
d(1910,1,1,4,42,40),
d(1918,9,1,5,0,0),
d(1919,7,2,4,0,0),
d(1927,9,1,5,0,0),
d(1928,4,1,4,0,0),
d(1928,9,1,5,0,0),
d(1929,4,1,4,0,0),
d(1929,9,1,5,0,0),
d(1930,4,1,4,0,0),
d(1930,9,1,5,0,0),
d(1931,4,1,4,0,0),
d(1931,9,1,5,0,0),
d(1932,4,1,4,0,0),
d(1932,9,1,5,0,0),
d(1966,10,9,4,0,0),
d(1967,3,12,3,0,0),
d(1967,10,15,4,0,0),
d(1968,3,10,3,0,0),
d(1968,10,13,4,0,0),
d(1969,3,9,3,0,0),
d(1969,10,12,4,0,0),
d(1970,3,15,3,0,0),
d(1970,10,11,4,0,0),
d(1971,3,14,3,0,0),
d(1971,10,10,4,0,0),
d(1972,3,12,3,0,0),
d(1972,10,15,4,0,0),
d(1973,3,11,3,0,0),
d(1973,10,14,4,0,0),
d(1974,3,10,3,0,0),
d(1974,10,13,4,0,0),
d(1975,3,9,3,0,0),
d(1975,10,12,4,0,0),
d(1976,3,14,3,0,0),
d(1976,10,10,4,0,0),
d(1977,3,13,3,0,0),
d(1977,10,9,4,0,0),
d(1978,3,12,3,0,0),
d(1978,10,15,4,0,0),
d(1979,3,11,3,0,0),
d(1979,10,14,4,0,0),
d(1980,3,9,3,0,0),
d(1980,10,12,4,0,0),
d(1981,3,15,3,0,0),
d(1981,10,11,4,0,0),
d(1982,3,14,3,0,0),
d(1982,10,10,4,0,0),
d(1983,3,13,3,0,0),
d(1983,10,9,4,0,0),
d(1984,3,11,3,0,0),
d(1984,10,14,4,0,0),
d(1985,3,10,3,0,0),
d(1985,10,13,4,0,0),
d(1986,3,9,3,0,0),
d(1986,10,12,4,0,0),
d(1987,3,15,3,0,0),
d(1987,10,11,4,0,0),
d(1988,3,13,3,0,0),
d(1988,10,9,4,0,0),
d(1989,3,12,3,0,0),
d(1989,10,15,4,0,0),
d(1990,3,11,3,0,0),
d(1990,10,14,4,0,0),
d(1991,3,10,3,0,0),
d(1991,10,13,4,0,0),
d(1992,3,15,3,0,0),
d(1992,10,11,4,0,0),
d(1993,3,14,3,0,0),
d(1993,10,10,4,0,0),
d(1994,3,13,3,0,0),
d(1994,10,9,4,0,0),
d(1995,3,12,3,0,0),
d(1995,10,15,4,0,0),
d(1996,3,10,3,0,0),
d(1996,10,13,4,0,0),
d(1997,3,9,3,0,0),
d(1997,10,12,4,0,0),
d(1998,3,15,3,0,0),
d(1998,9,27,4,0,0),
d(1999,4,4,3,0,0),
d(1999,10,10,4,0,0),
d(2000,3,12,3,0,0),
d(2000,10,15,4,0,0),
d(2001,3,11,3,0,0),
d(2001,10,14,4,0,0),
d(2002,3,10,3,0,0),
d(2002,10,13,4,0,0),
d(2003,3,9,3,0,0),
d(2003,10,12,4,0,0),
d(2004,3,14,3,0,0),
d(2004,10,10,4,0,0),
d(2005,3,13,3,0,0),
d(2005,10,9,4,0,0),
d(2006,3,12,3,0,0),
d(2006,10,15,4,0,0),
d(2007,3,11,3,0,0),
d(2007,10,14,4,0,0),
d(2008,3,9,3,0,0),
d(2008,10,12,4,0,0),
d(2009,3,15,3,0,0),
d(2009,10,11,4,0,0),
d(2010,3,14,3,0,0),
d(2010,10,10,4,0,0),
d(2011,3,13,3,0,0),
d(2011,10,9,4,0,0),
d(2012,3,11,3,0,0),
d(2012,10,14,4,0,0),
d(2013,3,10,3,0,0),
d(2013,10,13,4,0,0),
d(2014,3,9,3,0,0),
d(2014,10,12,4,0,0),
d(2015,3,15,3,0,0),
d(2015,10,11,4,0,0),
d(2016,3,13,3,0,0),
d(2016,10,9,4,0,0),
d(2017,3,12,3,0,0),
d(2017,10,15,4,0,0),
d(2018,3,11,3,0,0),
d(2018,10,14,4,0,0),
d(2019,3,10,3,0,0),
d(2019,10,13,4,0,0),
d(2020,3,15,3,0,0),
d(2020,10,11,4,0,0),
d(2021,3,14,3,0,0),
d(2021,10,10,4,0,0),
d(2022,3,13,3,0,0),
d(2022,10,9,4,0,0),
d(2023,3,12,3,0,0),
d(2023,10,15,4,0,0),
d(2024,3,10,3,0,0),
d(2024,10,13,4,0,0),
d(2025,3,9,3,0,0),
d(2025,10,12,4,0,0),
d(2026,3,15,3,0,0),
d(2026,10,11,4,0,0),
d(2027,3,14,3,0,0),
d(2027,10,10,4,0,0),
d(2028,3,12,3,0,0),
d(2028,10,15,4,0,0),
d(2029,3,11,3,0,0),
d(2029,10,14,4,0,0),
d(2030,3,10,3,0,0),
d(2030,10,13,4,0,0),
d(2031,3,9,3,0,0),
d(2031,10,12,4,0,0),
d(2032,3,14,3,0,0),
d(2032,10,10,4,0,0),
d(2033,3,13,3,0,0),
d(2033,10,9,4,0,0),
d(2034,3,12,3,0,0),
d(2034,10,15,4,0,0),
d(2035,3,11,3,0,0),
d(2035,10,14,4,0,0),
d(2036,3,9,3,0,0),
d(2036,10,12,4,0,0),
d(2037,3,15,3,0,0),
d(2037,10,11,4,0,0),
        ]

    _transition_info = [
i(-16980,0,'SMT'),
i(-18000,0,'CLT'),
i(-14400,3600,'CLST'),
i(-18000,0,'CLT'),
i(-14400,3600,'CLST'),
i(-18000,0,'CLT'),
i(-14400,3600,'CLST'),
i(-18000,0,'CLT'),
i(-14400,3600,'CLST'),
i(-18000,0,'CLT'),
i(-14400,3600,'CLST'),
i(-18000,0,'CLT'),
i(-14400,3600,'CLST'),
i(-18000,0,'CLT'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
i(-14400,0,'CLT'),
i(-10800,3600,'CLST'),
        ]

Continental = Continental()

