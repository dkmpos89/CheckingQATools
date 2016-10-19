rem cd C:\Users\diego.campos\Pictures\
set FILE="C:\Users\diego.campos\Pictures\imagen.JPG"

if exist %FILE% rename %FILE%  "new.JPG"

pause>nul
exit