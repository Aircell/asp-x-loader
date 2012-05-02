.SILENT:

default:
	echo "Please switch to a hardware-specific branch (either p2 or p3)."
	echo
	echo "To see what branches you already have: 'git branch -l'"
	echo
	echo "If you don't already have those branches locally:"
	echo "  Switching to p2: 'git checkout -b p2 origin/p2'"
	echo "  Switching to p3: 'git checkout -b p3 origin/p3'"
