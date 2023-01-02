#########################################################
# Substance Framework UTF8-CPP
#########################################################

# UTF8-CPP files
set (UTF8CPP_SOURCES
    source/utf8.h
	source/utf8/checked.h
	source/utf8/core.h
	source/utf8/unchecked.h
)

autogen_source_groups("${UTF8CPP_SOURCES}")

add_library(utf8cpp INTERFACE ${UTF8CPP_SOURCES})