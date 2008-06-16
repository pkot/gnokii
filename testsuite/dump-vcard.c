#include <gnokii.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main (int argc, char **argv)
{
	FILE *f;
	gn_phonebook_entry entry;
	char *buf, *vcard;

	if (argc < 2)
		return 1;

	/* Open the file */
	f = fopen (argv[1], "r");
	if (f == NULL) {
		perror ("Opening failed");
		return 1;
	}

	/* Parse using gn_vcardstr2phonebook() */
	memset (&entry, 0, sizeof(entry));
	if (gn_vcard2phonebook (f, &entry) < 0) {
		fprintf (stderr, "Parsing '%s' failed\n", argv[1]);
		return 1;
	}

	/* Seek back to the beginning and read in memory */
	if (fseek (f, 0, SEEK_SET) < 0) {
		perror ("Seeking back failed");
		return 1;
	}

	/* XXX HACK */
	buf = malloc (1024 * 1024);
	if (buf == NULL) {
		fprintf (stderr, "Couldn't allocate a meg of memory\n");
		return 1;
	}
	if (fread (buf, 1024 * 1024, 1, f) < 0) {
		perror ("Failed to read");
		return 1;
	}

	fclose (f);

	/* Parse using gn_vcardstr2phonebook() */
	memset (&entry, 0, sizeof(entry));
	if (gn_vcardstr2phonebook (buf, &entry) < 0) {
		fprintf (stderr, "Parsing '%s' failed\n", argv[1]);
		return 1;
	}
	free (buf);

	/* And now back */
	vcard = gn_phonebook2vcardstr (&entry);
	if (vcard == NULL) {
		fprintf (stderr, "Failed to parse back '%s'\n", argv[1]);
		return 1;
	}

	printf ("%s\n", vcard);
	free (vcard);

	return 0;
}
