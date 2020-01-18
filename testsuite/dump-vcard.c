#include "compat.h"
#include "gnokii.h"

int main (int argc, char **argv)
{
	FILE *f;
	gn_phonebook_entry entry;
	char *buf, *vcard;
	int is_stdin;

	if (argc != 2) {
		fprintf (stderr, "Usage: %s \"stdin\"|FILENAME\n", argv[0]);
		return 1;
	}

	if (strcmp (argv[1], "stdin") == 0)
		is_stdin = 1;
	else
		is_stdin = 0;

	/* Open the file */
	if (!is_stdin) {
		f = fopen (argv[1], "r");
		if (f == NULL) {
			perror ("Opening failed");
			return 1;
		}
	} else {
		f = stdin;
	}

	/* Parse using gn_vcardstr2phonebook() */
	memset (&entry, 0, sizeof(entry));
	if (gn_vcard2phonebook (f, &entry) < 0) {
		fprintf (stderr, "Parsing '%s' failed\n", argv[1]);
		if (!is_stdin)
			fclose (f);
		return 1;
	}

	if (!is_stdin) {
		size_t count;

		/* Seek back to the beginning and read in memory */
		if (fseek (f, 0, SEEK_SET) < 0) {
			perror ("Seeking back failed");
			fclose (f);
			return 1;
		}

		/* XXX HACK */
		buf = malloc (1024 * 1024);
		if (buf == NULL) {
			fprintf (stderr, "Couldn't allocate a meg of memory\n");
			fclose (f);
			return 1;
		}
		count = fread (buf, 1, 1024 * 1024 - 1, f);
		if (count == 0) {
			perror ("Failed to read");
			free (buf);
			fclose (f);
			return 1;
		}
		buf[count] = '\0';

		fclose (f);

		/* Parse using gn_vcardstr2phonebook() */
		memset (&entry, 0, sizeof(entry));
		if (gn_vcardstr2phonebook (buf, &entry) < 0) {
			fprintf (stderr, "Parsing '%s' failed\n", argv[1]);
			free (buf);
			return 1;
		}
		free (buf);
	}

	/* And now back */
	vcard = gn_phonebook2vcardstr (&entry);
	if (vcard == NULL) {
		fprintf (stderr, "Failed to parse back '%s'\n", argv[1]);
		return 1;
	}

	printf ("%s", vcard);
	free (vcard);

	return 0;
}
