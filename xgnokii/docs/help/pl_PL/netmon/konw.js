var konw='0'; //jezeli na dysku jest www.fkn.pl off-line, nalezy wgrac
             //do katalogu marcinw i zmienic ten parameter na 1
             //(wtedy wszystkie odnosniki do www.fkn.pl beda zmieniane
             //na odnosniki do lokalnych plikow)

if (konw=='1')
{
  if (document.location.protocol=="file:")
  {
    for (var i=0;i<document.links.length;++i)
    {
      if (document.links[i].hostname=='www.fkn.pl')
      {
        var s=document.links[i].href.substring(18,document.links[i].href.length);
        if (s!='') {document.links[i].href='../../'+s}
              else {document.links[i].href='../../'+s+'index.html'}
      }
    }
  }
}
