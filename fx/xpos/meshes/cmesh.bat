@if exist %1.gem goto convertgem
@if exist %1.jxx goto convertjxx
@if exist %1.3d goto convert3d
@goto notfound

:convertgem
@del ..\%1.som
@gem2som %1.gem ..\%1.som %2 %3 %4 %5 %6 %7
@goto end

:convert3d
@del ..\%1.som
@3d2som %1.3d ..\%1.som %2 %3 %4 %5 %6 %7
@goto end

:convertjxx
@del ..\%1.som
@jxx2som %1.jxx ..\%1.som %2 %3 %4 %5 %6 %7

:end
@goto endend

:notfound
@echo there is no file called %1.jxx/%1.3d/%1.gem.

:endend
