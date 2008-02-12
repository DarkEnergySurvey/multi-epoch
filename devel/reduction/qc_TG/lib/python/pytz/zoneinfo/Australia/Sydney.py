'''tzinfo timezone information for Australia/Sydney.'''
from pytz.tzinfo import DstTzInfo
from pytz.tzinfo import memorized_datetime as d
from pytz.tzinfo import memorized_ttinfo as i

class Sydney(DstTzInfo):
    '''Australia/Sydney timezone definition. See datetime.tzinfo for details'''

    _zone = 'Australia/Sydney'

    _utc_transition_times = [
d(1,1,1,0,0,0),
d(1916,12,31,14,1,0),
d(1917,3,24,15,0,0),
d(1941,12,31,16,0,0),
d(1942,3,28,15,0,0),
d(1942,9,26,16,0,0),
d(1943,3,27,15,0,0),
d(1943,10,2,16,0,0),
d(1944,3,25,15,0,0),
d(1971,10,30,16,0,0),
d(1972,2,26,16,0,0),
d(1972,10,28,16,0,0),
d(1973,3,3,16,0,0),
d(1973,10,27,16,0,0),
d(1974,3,2,16,0,0),
d(1974,10,26,16,0,0),
d(1975,3,1,16,0,0),
d(1975,10,25,16,0,0),
d(1976,3,6,16,0,0),
d(1976,10,30,16,0,0),
d(1977,3,5,16,0,0),
d(1977,10,29,16,0,0),
d(1978,3,4,16,0,0),
d(1978,10,28,16,0,0),
d(1979,3,3,16,0,0),
d(1979,10,27,16,0,0),
d(1980,3,1,16,0,0),
d(1980,10,25,16,0,0),
d(1981,2,28,16,0,0),
d(1981,10,24,16,0,0),
d(1982,4,3,16,0,0),
d(1982,10,30,16,0,0),
d(1983,3,5,16,0,0),
d(1983,10,29,16,0,0),
d(1984,3,3,16,0,0),
d(1984,10,27,16,0,0),
d(1985,3,2,16,0,0),
d(1985,10,26,16,0,0),
d(1986,3,15,16,0,0),
d(1986,10,18,16,0,0),
d(1987,3,14,16,0,0),
d(1987,10,24,16,0,0),
d(1988,3,19,16,0,0),
d(1988,10,29,16,0,0),
d(1989,3,18,16,0,0),
d(1989,10,28,16,0,0),
d(1990,3,3,16,0,0),
d(1990,10,27,16,0,0),
d(1991,3,2,16,0,0),
d(1991,10,26,16,0,0),
d(1992,2,29,16,0,0),
d(1992,10,24,16,0,0),
d(1993,3,6,16,0,0),
d(1993,10,30,16,0,0),
d(1994,3,5,16,0,0),
d(1994,10,29,16,0,0),
d(1995,3,4,16,0,0),
d(1995,10,28,16,0,0),
d(1996,3,30,16,0,0),
d(1996,10,26,16,0,0),
d(1997,3,29,16,0,0),
d(1997,10,25,16,0,0),
d(1998,3,28,16,0,0),
d(1998,10,24,16,0,0),
d(1999,3,27,16,0,0),
d(1999,10,30,16,0,0),
d(2000,3,25,16,0,0),
d(2000,8,26,16,0,0),
d(2001,3,24,16,0,0),
d(2001,10,27,16,0,0),
d(2002,3,30,16,0,0),
d(2002,10,26,16,0,0),
d(2003,3,29,16,0,0),
d(2003,10,25,16,0,0),
d(2004,3,27,16,0,0),
d(2004,10,30,16,0,0),
d(2005,3,26,16,0,0),
d(2005,10,29,16,0,0),
d(2006,3,25,16,0,0),
d(2006,10,28,16,0,0),
d(2007,3,24,16,0,0),
d(2007,10,27,16,0,0),
d(2008,3,29,16,0,0),
d(2008,10,25,16,0,0),
d(2009,3,28,16,0,0),
d(2009,10,24,16,0,0),
d(2010,3,27,16,0,0),
d(2010,10,30,16,0,0),
d(2011,3,26,16,0,0),
d(2011,10,29,16,0,0),
d(2012,3,24,16,0,0),
d(2012,10,27,16,0,0),
d(2013,3,30,16,0,0),
d(2013,10,26,16,0,0),
d(2014,3,29,16,0,0),
d(2014,10,25,16,0,0),
d(2015,3,28,16,0,0),
d(2015,10,24,16,0,0),
d(2016,3,26,16,0,0),
d(2016,10,29,16,0,0),
d(2017,3,25,16,0,0),
d(2017,10,28,16,0,0),
d(2018,3,24,16,0,0),
d(2018,10,27,16,0,0),
d(2019,3,30,16,0,0),
d(2019,10,26,16,0,0),
d(2020,3,28,16,0,0),
d(2020,10,24,16,0,0),
d(2021,3,27,16,0,0),
d(2021,10,30,16,0,0),
d(2022,3,26,16,0,0),
d(2022,10,29,16,0,0),
d(2023,3,25,16,0,0),
d(2023,10,28,16,0,0),
d(2024,3,30,16,0,0),
d(2024,10,26,16,0,0),
d(2025,3,29,16,0,0),
d(2025,10,25,16,0,0),
d(2026,3,28,16,0,0),
d(2026,10,24,16,0,0),
d(2027,3,27,16,0,0),
d(2027,10,30,16,0,0),
d(2028,3,25,16,0,0),
d(2028,10,28,16,0,0),
d(2029,3,24,16,0,0),
d(2029,10,27,16,0,0),
d(2030,3,30,16,0,0),
d(2030,10,26,16,0,0),
d(2031,3,29,16,0,0),
d(2031,10,25,16,0,0),
d(2032,3,27,16,0,0),
d(2032,10,30,16,0,0),
d(2033,3,26,16,0,0),
d(2033,10,29,16,0,0),
d(2034,3,25,16,0,0),
d(2034,10,28,16,0,0),
d(2035,3,24,16,0,0),
d(2035,10,27,16,0,0),
d(2036,3,29,16,0,0),
d(2036,10,25,16,0,0),
d(2037,3,28,16,0,0),
d(2037,10,24,16,0,0),
        ]

    _transition_info = [
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
i(36000,0,'EST'),
i(39600,3600,'EST'),
        ]

Sydney = Sydney()

