@if exist %1.id goto custom
@edit test.id
@del test.wld
@playgod test.id test.wld
@goto end
:custom
@edit %1.id
@del %1.wld
@playgod %1.id %1.wld
:end
@echo New World created. (whoops... didn't take 7 days, did it ?)

