config() {
  NEW="$1"
  OLD="`dirname $NEW`/`basename $NEW .new`"
  # If there's no config file by that name, mv it over:
  if [ ! -r $OLD ]; then
    mv $NEW $OLD
  elif [ "`cat $OLD | md5sum`" = "`cat $NEW | md5sum`" ]; then # toss the redundant copy
    rm $NEW
  fi
  # Otherwise, we leave the .new copy for the admin to consider...
}
permissions() {
  if [ x`grep gnokii /etc/group | grep -v ^#` = "x" ]; then
    /usr/sbin/groupadd gnokii
  fi
  chown root.gnokii /usr/bin/gnokii
  chmod 750 /usr/bin/gnokii
  chown root.gnokii /usr/sbin/gnokiid
  chmod 750 /usr/sbin/gnokiid
  chown root.gnokii /usr/sbin/mgnokiidev
  chmod 4750 /usr/sbin/mgnokiidev
  if [ -f /usr/X11R6/bin/xgnokii ]; then
    chown root.gnokii /usr/X11R6/bin/xgnokii
    chmod 750 /usr/X11R6/bin/xgnokii
  fi
}
config etc/gnokiirc.new
permissions
