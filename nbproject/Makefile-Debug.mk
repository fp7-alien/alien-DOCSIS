#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/src/DOCSIS/CMTS/CMTS_handler.o \
	${OBJECTDIR}/src/DOCSIS/DOCSISdriver.o \
	${OBJECTDIR}/src/QoS/QoS.o \
	${OBJECTDIR}/src/discovery/discovery.o \
	${OBJECTDIR}/src/main.o \
	${OBJECTDIR}/src/orchestrator/Flowcache.o \
	${OBJECTDIR}/src/orchestrator/orchestrator.o \
	${OBJECTDIR}/src/proxy/ALHINP.o \
	${OBJECTDIR}/src/translator/translator.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/alhinp

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/alhinp: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/alhinp ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/src/DOCSIS/CMTS/CMTS_handler.o: src/DOCSIS/CMTS/CMTS_handler.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/DOCSIS/CMTS
	${RM} "$@.d"
	$(COMPILE.cc) -g -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/DOCSIS/CMTS/CMTS_handler.o src/DOCSIS/CMTS/CMTS_handler.cpp

${OBJECTDIR}/src/DOCSIS/DOCSISdriver.o: src/DOCSIS/DOCSISdriver.c++ 
	${MKDIR} -p ${OBJECTDIR}/src/DOCSIS
	${RM} "$@.d"
	$(COMPILE.cc) -g -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/DOCSIS/DOCSISdriver.o src/DOCSIS/DOCSISdriver.c++

${OBJECTDIR}/src/QoS/QoS.o: src/QoS/QoS.c++ 
	${MKDIR} -p ${OBJECTDIR}/src/QoS
	${RM} "$@.d"
	$(COMPILE.cc) -g -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/QoS/QoS.o src/QoS/QoS.c++

${OBJECTDIR}/src/discovery/discovery.o: src/discovery/discovery.c++ 
	${MKDIR} -p ${OBJECTDIR}/src/discovery
	${RM} "$@.d"
	$(COMPILE.cc) -g -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/discovery/discovery.o src/discovery/discovery.c++

${OBJECTDIR}/src/main.o: src/main.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/main.o src/main.cpp

${OBJECTDIR}/src/orchestrator/Flowcache.o: src/orchestrator/Flowcache.c++ 
	${MKDIR} -p ${OBJECTDIR}/src/orchestrator
	${RM} "$@.d"
	$(COMPILE.cc) -g -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/orchestrator/Flowcache.o src/orchestrator/Flowcache.c++

${OBJECTDIR}/src/orchestrator/orchestrator.o: src/orchestrator/orchestrator.c++ 
	${MKDIR} -p ${OBJECTDIR}/src/orchestrator
	${RM} "$@.d"
	$(COMPILE.cc) -g -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/orchestrator/orchestrator.o src/orchestrator/orchestrator.c++

${OBJECTDIR}/src/proxy/ALHINP.o: src/proxy/ALHINP.c++ 
	${MKDIR} -p ${OBJECTDIR}/src/proxy
	${RM} "$@.d"
	$(COMPILE.cc) -g -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/proxy/ALHINP.o src/proxy/ALHINP.c++

${OBJECTDIR}/src/translator/translator.o: src/translator/translator.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/translator
	${RM} "$@.d"
	$(COMPILE.cc) -g -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/translator/translator.o src/translator/translator.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/alhinp

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
