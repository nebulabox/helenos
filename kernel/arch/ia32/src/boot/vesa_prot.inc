#ifdef CONFIG_FB

#define MULTIBOOT_LOADER_MAGIC  0x2BADB002
#define MBINFO_BIT_CMDLINE      2
#define MBINFO_OFFSET_CMDLINE   16

	# copy real mode VESA initialization code
	
	mov $vesa_init, %esi
	mov $VESA_INIT_SEGMENT << 4, %edi
	mov $e_vesa_init - vesa_init, %ecx
	rep movsb
	
	# check for GRUB command line
	
	mov grub_eax, %eax
	cmp $MULTIBOOT_LOADER_MAGIC, %eax
	jne no_cmdline
	
	mov grub_ebx, %ebx
	mov (%ebx), %eax
	bt $MBINFO_BIT_CMDLINE, %eax
	jnc no_cmdline
	
	# skip the kernel path in command line
	
	mov MBINFO_OFFSET_CMDLINE(%ebx), %esi
	
	skip_loop:
		lodsb
		
		cmp $0, %al
		je no_cmdline
		
		cmp $' ', %al
		je skip_loop_done
		
		jmp skip_loop
	skip_loop_done:
	
	mov (%esi), %al
	cmp $0, %al
	je no_cmdline
	
	# copy at most 23 characters from command line
	
	mov $VESA_INIT_SEGMENT << 4, %edi
	add $default_mode - vesa_init, %edi
	mov $23, %ecx
	
	cmd_loop:
		lodsb
		stosb
		
		cmp $0, %al
		je cmd_loop_done
		
		loop cmd_loop
	cmd_loop_done:
	
	# zero termination
	
	xor %eax, %eax
	stosb
	
	no_cmdline:
	
	# jump to the real mode
	
	mov $VESA_INIT_SEGMENT << 4, %edi
	jmpl *%edi
	
	vesa_meeting_point:
		# returned back to protected mode
		
		mov %esi, KA2PA(vesa_ph_addr)
		mov %di, KA2PA(vesa_height)
		shr $16, %edi
		mov %di, KA2PA(vesa_width)
		mov %bx, KA2PA(vesa_scanline)
		shr $16, %ebx
		mov %bx, KA2PA(vesa_bpp)
#endif