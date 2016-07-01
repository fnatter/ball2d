#!perl -w
$i = 0;
while (defined($oldname = $ARGV[$i])) {
    $newname = $oldname;
    $newname =~ tr/A-Z/a-z/;
    rename($oldname, $newname);
    $i++;
}
