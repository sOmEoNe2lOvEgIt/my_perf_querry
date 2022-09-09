LIBIBMAD_DIR = /usr/lib64
SLURM_INC_DIR = /root/SLURM/slurm.build
SLURM_BUILD = 21.08.8-2
SLURM_BUILD_DIR = /root/rpmbuild/BUILD/slurm-$(SLURM_BUILD)
NAME = perf_querry

SRC_FILES = perf_querry.c	  	\
			querry.c			\

CC	  = gcc
CFLAGS  ?= -Wall -g -Iinclude -I$(SLURM_INC_DIR) -I$(SLURM_BUILD_DIR) -I$(DEMETER_LIB_DIR)/include -I/usr/include/infiniband/
LDFLAGS ?= -L$(LIBIBMAD_DIR) -libmad

all: $(NAME)

default: $(NAME)

$(NAME): $(SRC_FILES)
		$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

send: all
		scp $(NAME) my_vm:/home/compose_fake_taranis/shared/

clean:
		rm -f $(NAME)

re: clean all