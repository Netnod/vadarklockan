vroughtime - a roughtime client in C
====================================

Vad är klockan relies on the project `vroughtime
<https://github.com/oreparaz/vroughtime>`_ by Oscar Reparaz for the
client implementation of the roughtime protocol.  It has been modified
to support `IETF draft version 06 of Roughtime
<https://datatracker.ietf.org/doc/html/draft-ietf-ntp-roughtime-06>`_.  To do
that the hardcoded query string has been replaced with a function to
build a query dynamically.

.. doxygenfunction:: vrt_make_query

.. doxygenfunction:: vrt_parse_response
