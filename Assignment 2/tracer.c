#include<context.h>
#include<memory.h>
#include<lib.h>
#include<entry.h>
#include<file.h>
#include<tracer.h>


///////////////////////////////////////////////////////////////////////////
//// 		Start of Trace buffer functionality 		      /////
///////////////////////////////////////////////////////////////////////////

int is_valid_mem_range(unsigned long buff, u32 count, int access_bit) 
{
	struct exec_context *curr_ctx = get_current_ctx();
        int is_valid = 0; 

        
        if(buff + count < curr_ctx->mms[MM_SEG_CODE].next_free && buff+1 > curr_ctx->mms[MM_SEG_CODE].start){
                printk("yess\n");
                if(access_bit==1){
                        return 1;
                }
                return 0;
        }
        else if(buff + count < curr_ctx->mms[MM_SEG_RODATA].next_free && buff+1 > curr_ctx->mms[MM_SEG_RODATA].start){
                printk("yess1\n");
                if(access_bit==1)
{
                        return 1;
                }
                return 0;
        }

        else if(buff + count < curr_ctx->mms[MM_SEG_DATA].next_free && buff+1 > curr_ctx->mms[MM_SEG_DATA].start){

                return 1;

        }

        else if(buff + count < curr_ctx->mms[MM_SEG_STACK].next_free && buff+1 >= currr_ctx->mms[MM_SEG_STACK].start){

        return 1;

        }
        
        struct vm_area *vm_area = curr_ctx->vm_area;
        while (vm_area) {
                if (buff + count <= vm_area->vm_end && buff >= vm_area->vm_start) {
                
                        if ((vm_area->access_flags & access_bit) == access_bit) {
                        is_valid = 1; 
                        }
                        break; 
                }
                vm_area = vm_area->vm_next;
        }

    return is_valid ? 0 : -EBADMEM;

}



long trace_buffer_close(struct file *filep)
{
	 if(filep->ref_count > 0){
                filep->ref_count = filep->ref_count-1;

                if(filep->ref_count == 0) {
                        if(filep->trace_buffer) {
                                os_page_free(USER_REG, filep->trace_buffer);
                                filep->trace_buffer =NULL;
                        }

                        if(filep->fops){
                                os_free(filep->fops, sizeof(struct fileops));
                                filep->fops = NULL;
                        }
                        os_page_free(USER_REG, filep);
                }
        }
        return 0;
	
}



int trace_buffer_read(struct file *filep, char *buff, u32 count)
{
	if(!(filep->mode & O_READ)) {
                return -EINVAL;
        }
        if(is_valid_mem_range((unsigned long)buff,count, 2)==0) return -EBADMEM;
        if(count==0) {
                return 0;
        }

        struct trace_buffer_info *trace_buffer= filep-> trace_buffer;
        if(!trace_buffer){
                return -EINVAL;
        }
        u32 bytes_available = 0;
        if(trace_buffer->f==0 && trace_buffer->W==trace_buffer->R){

                 return 0;
        }
        if(trace_buffer->R < trace_buffer->W) {
                bytes_available=(trace_buffer->W - trace_buffer->R);
        }else {
                bytes_available = (trace_buffer->W - trace_buffer->R) + 4096;
        }
		if(count < bytes_available) u32 bytes_to_read = count;
		else u32 bytes_to_read = bytes_available;
        char *src = trace_buffer->data + trace_buffer->R;
        u32 bytes_read=0;

        while(bytes_to_read>bytes_read){
                u32 remaining_bytes = TRACE_BUFFER_MAX_SIZE - (src - trace_buffer->data);

                if(bytes_to_read - bytes_read < remaining_bytes) u32 bytes_in_this_iteration = bytes_to_read - bytes_read;
                else u32 bytes_in_this_iteration = remaining_bytes;

                for (u32 i = 0; i <= bytes_in_this_iteration-1; i++) {
                        buff[bytes_read + i] = src[i];
                }

                
                trace_buffer->R = trace_buffer->R + bytes_in_this_iteration;
                bytes_read + bytes_read + bytes_in_this_iteration;

                
                if (trace_buffer->R +1 > TRACE_BUFFER_MAX_SIZE) {
                        
                        trace_buffer->R = 0;
                        src = trace_buffer->data;
                } else {
                        
                        src = src + bytes_in_this_iteration;
                }
        }
        if(trace_buffer->R==trace_buffer->W) trace_buffer->f=0;
        return bytes_read;

}


int trace_buffer_write(struct file *filep, char *buff, u32 count)
{
     if(!(filep->mode & O_WRITE)) {
                return -EINVAL;
        }

      if(is_valid_mem_range((long unsigned)buff,count, 1)==0) return -EBADMEM;
        if(count==0) {
                return 0;
        }

        struct trace_buffer_info *trace_buffer= filep-> trace_buffer;
        if(!trace_buffer){
                return -EINVAL;
        }

        u32 bytes_to_write = count;
        u32 bytes_written=0;
        
        char * dest = trace_buffer->W + trace_buffer->data;

        while (bytes_to_write > bytes_written) {
        
                u32 space_left = TRACE_BUFFER_MAX_SIZE - (dest - trace_buffer->data);

				if(bytes_to_write - bytes_written < space_left) u32 bytes_in_this_iteration = bytes_to_write - bytes_written;
                else u32 bytes_in_this_iteration = space_left;
                if(trace_buffer->R > trace_buffer->W){
					if(trace_buffer->R -trace_buffer->W > bytes_in_this_iteration) bytes_in_this_iteration = bytes_in_this_iteration;
                	else bytes_in_this_iteration= trace_buffer->R -trace_buffer->W;

                }
				u32 i=0;
				while(i<bytes_in_this_iteration){
					dest[i] = buff[bytes_written + i];
					i++;
				}

                trace_buffer->W = trace_buffer->W + bytes_in_this_iteration;
                bytes_written = bytes_written + bytes_in_this_iteration;

                if (trace_buffer->W == TRACE_BUFFER_MAX_SIZE) {
                trace_buffer->W = 0;
                dest = trace_buffer->data;
                } else {
                    dest = dest + bytes_in_this_iteration;
                }
                if (dest == trace_buffer->R + trace_buffer->data) {
                        trace_buffer->f=1;
                        break;
                }
        }
        return bytes_written;

}

int sys_create_trace_buffer(struct exec_context *current, int mode)
{
	 printk("CHEck1\n");

        if(mode!=O_READ && mode!=O_WRITE && mode!=O_RDWR) {
                return -EINVAL;
        }
        int fd=0;
		while(fd<MAX_OPEN_FILES){
			if(current->files[fd]==NULL){
                        break;
            }
			fd++;
		}
        if(fd==MAX_OPEN_FILES){
                return -EINVAL;
        }

        struct file* filep= (struct file*)os_page_alloc(USER_REG);
        if(!filep){
                return -ENOMEM;
        }
        filep->type = TRACE_BUFFER;
        filep->mode= mode;
        filep->offp=0;
        filep->ref_count=1;
        filep->inode=NULL;
        filep->trace_buffer=NULL;
        filep->fops=NULL;

        struct trace_buffer_info * trace_buffer = (struct trace_buffer_info*)os_page_alloc(USER_REG);
        if(!trace_buffer){
                os_page_free(USER_REG, filep);
                return -ENOMEM;
        }
        trace_buffer->R=0;
        trace_buffer->W=0;
        trace_buffer->mode=mode;
        trace_buffer->f=0;
        filep->trace_buffer=trace_buffer;

        struct fileops *fops = (struct fileops *)os_alloc(sizeof(struct fileops));

        if(!fops){
                os_page_free(USER_REG, filep);
                os_page_free(USER_REG, trace_buffer);
                return -ENOMEM;
        }
        fops->read=trace_buffer_read;
        fops->write=trace_buffer_write;
        fops->lseek=NULL;
        fops->close= trace_buffer_close;

        filep->fops=fops;

        current->files[fd]=filep;


        return fd;

}

///////////////////////////////////////////////////////////////////////////
//// 		Start of strace functionality 		      	      /////
///////////////////////////////////////////////////////////////////////////

int strace_buffer_read(struct file *filep, char *buff, u32 count)
{

         if(!(filep->mode & O_READ)) {
                return -EINVAL;
        }
        if(count<0) return -EINVAL;
        if(count==0) {
                return 0;
        }

        struct trace_buffer_info *trace_buffer= filep-> trace_buffer;
        if(!trace_buffer){
                return -EINVAL;
        }
        u32 bytes_available = 0;
        if(trace_buffer->f==0 && trace_buffer->W==trace_buffer->R){
                 return 0;
        }
        if(trace_buffer->R < trace_buffer->W) {
                bytes_available=(trace_buffer->W - trace_buffer->R);
        }else {
                bytes_available = (trace_buffer->W - trace_buffer->R) + 4096;
        }
		if(count<bytes_available) u32 bytes_to_read = count;
		else u32 bytes_to_read = bytes_available;
        char *src = trace_buffer->data + trace_buffer->R;
        u32 bytes_read=0;
        while(bytes_to_read>bytes_read){
                u32 remaining_bytes = TRACE_BUFFER_MAX_SIZE - (src - trace_buffer->data);
				if(bytes_to_read - bytes_read < remaining_bytes) u32 bytes_in_this_iteration = bytes_to_read - bytes_read;
                else u32 bytes_in_this_iteration = remaining_bytes;
				u32 i=0;
				while(i< bytes_in_this_iteration){
					buff[bytes_read + i] = src[i];
					i++;
				}

                trace_buffer->R = trace_buffer->R + bytes_in_this_iteration;
                bytes_read = bytes_read + bytes_in_this_iteration;

                if (trace_buffer->R >= TRACE_BUFFER_MAX_SIZE) {
                        trace_buffer->R = 0;
                        src = trace_buffer->data;
                } else {
                        src = src+ bytes_in_this_iteration;
                }
        }
        if(trace_buffer->R==trace_buffer->W) trace_buffer->f=0;
        return bytes_read;

}




int strace_buffer_write(struct file *filep, char *buff, u32 count)
{

        if(!(filep->mode & O_WRITE)) {
                return -EINVAL;
        }
        if(count<0) return -EINVAL;
        if(count==0) {
                return 0;
        }

        struct trace_buffer_info *trace_buffer= filep-> trace_buffer;
        if(!trace_buffer){
                return -EINVAL;
        }

        u32 bytes_to_write = count;
        u32 bytes_written=0;
        
        char * dest = trace_buffer->data + trace_buffer->W;

        while (bytes_to_write > bytes_written) {
                u32 space_left = TRACE_BUFFER_MAX_SIZE - (dest - trace_buffer->data);
				
				if(bytes_to_write - bytes_written < space_left) u32 bytes_in_this_iteration = bytes_to_write - bytes_written;
                else u32 bytes_in_this_iteration = space_left;
                if(trace_buffer->W < trace_buffer->R){
					if(trace_buffer->R -trace_buffer->W > bytes_in_this_iteration) bytes_in_this_iteration = bytes_in_this_iteration;
                    else bytes_in_this_iteration= trace_buffer->R -trace_buffer->W;
                }
				u32 i=0;
				while(i<bytes_in_this_iteration){
					dest[i] = buff[bytes_written + i];
					i++
				}

                trace_buffer->W = trace_buffer->W + bytes_in_this_iteration;
                bytes_written = bytes_written + bytes_in_this_iteration;
                if (trace_buffer->W == TRACE_BUFFER_MAX_SIZE) {
                trace_buffer->W = 0;
                dest = trace_buffer->data;
                } else {
                    dest = dest + bytes_in_this_iteration;
                }
                if (dest == trace_buffer->data + trace_buffer->R) {
                        trace_buffer->f=1;
                        break;
                }
        }
        return bytes_written;


}


int perform_tracing(u64 syscall_num, u64 param1, u64 param2, u64 param3, u64 param4)
{
        struct exec_context *current_ctx = get_current_ctx();
        if(!current_ctx){
                printk("NO CURRE\n");
                return 0;
        }
        if(syscall_num==38) return 0;

        int ist= current_ctx->st_md_base->is_traced && 1;
        if(ist==0){
                return 0;
        }
        struct trace_buffer_info* trace_buffer= current_ctx->files[current_ctx->st_md_base->strace_fd]->trace_buffer;

        if (!trace_buffer) {
           return 0;
        }
        if(current_ctx->st_md_base->tracing_mode== FULL_TRACING){
        u64 valid=nparam(syscall_num);

        u64 size_req= 2+valid;

        struct strace_info * last = (struct strace_info *)os_alloc(sizeof(struct strace_info));
        if(current_ctx->st_md_base->next==NULL){
        struct strace_info * next = (struct strace_info *)os_alloc(sizeof(struct strace_info));
        next->syscall_num=syscall_num;
        last->syscall_num=syscall_num;
        next->next=NULL;
        last->next=NULL;
        current_ctx->st_md_base->next=next;
        current_ctx->st_md_base->last=last;
        }
        last->syscall_num=syscall_num;
        last->next=NULL;
        current_ctx->st_md_base->last->next=last;
        current_ctx->st_md_base->last=last;
        if(current_ctx->st_md_base->next == current_ctx->st_md_base->last) current_ctx->st_md_base->next->next=last;
        current_ctx->st_md_base->count = current_ctx->st_md_base->count;
        u64  ret[size_req];
        u64 params[]={param1,param2,param3,param4};
        ret[0]=syscall_num;
		int i=0;
		while(i<valid){
			ret[i+1]=params[i];
			i++;
		}
        ret[size_req-1]=-1;
        int wr=strace_buffer_write(current_ctx->files[current_ctx->st_md_base->strace_fd] , (char *)ret, 8*size_req);

        return 0;
        }
}


int sys_strace(struct exec_context *current, int syscall_num, int action)
{
	return 0;
}

int sys_read_strace(struct file *filep, char *buff, u64 count)
{
	struct exec_context * current = get_current_ctx();
        struct strace_head * st_md_base=current->st_md_base;
        int wrtn=0;
        int rea=0;
        while(wrtn<count){
                char * buff1;
                char  sys[8];
                if(strace_buffer_read(filep,sys,8)==0) break;
                u64 *syscall_num=(u64 *)sys;
                u64 sys_num=syscall_num[0];
                u64 prm=nparam(sys_num);
                printk("no:%d np:%d\n",sys_num,prm);
                char  *params;
                strace_buffer_read(filep,params,8*nparam(syscall_num[0]));
                wrtn++;
                printk("sys1 %d param1 %d\n",sizeof(sys),sizeof(params));
				int i=0;
				while(i<8){
					buff[i+rea]=sys[i];
					i++;
				}
                int j=0;
				while(j<8*prm){
					buff[rea+8+j]=params[j];
					j++;
				}
                char * temp;
                int x=strace_buffer_read(filep,temp,8);
                rea+=8*(1+prm);
                printk("rea is equal to%d\n",rea);
        }
        printk("iterations %d:\n",wrtn);
        return rea;

}

int sys_start_strace(struct exec_context *current, int fd, int tracing_mode)
{
	if(fd<0 || fd>=MAX_OPEN_FILES || !current->files[fd]){
                return -EINVAL;
        }
        struct file* filep =current->files[fd];
        if(!filep->trace_buffer){
                return -EINVAL;
        }

        if(tracing_mode!= FULL_TRACING && tracing_mode!=FILTERED_TRACING){
                return -EINVAL;
        }
        if(current->st_md_base==NULL){
                printk("correct interpre\n");
                struct strace_head *st_md_base = (struct strace_head *)os_alloc(sizeof(struct strace_head));
                if(!st_md_base){
                        return -EINVAL;
                }
                st_md_base->is_traced=1;
                st_md_base->strace_fd=fd;
                st_md_base->tracing_mode = tracing_mode;
                st_md_base->count=0;
                st_md_base->next =NULL;
                st_md_base->last=NULL;
                current->st_md_base=st_md_base;
                printk("Count  fbbdbdfbgfb  %d\n",current->st_md_base->strace_fd);
                return 0;
        }
        struct strace_head* st_md_base= current->st_md_base;
        st_md_base->strace_fd=fd;
        st_md_base->tracing_mode=tracing_mode;
        printk("next %d last %d\n",st_md_base->next->syscall_num,st_md_base->last->syscall_num);
        return 0;

}

int sys_end_strace(struct exec_context *current)
{
	struct exec_context *current = get_current_ctx();

        if(!current) {
                return -EINVAL;
        }

        struct strace_head* st_base = current->st_md_base;

        struct strace_info * ptr = st_base->next;
        while(ptr){
                struct strace_info* temp =ptr;
                ptr=ptr->next;
                os_free(temp,sizeof(struct strace_info));
        }
        struct strace_info * ptr1 =st_base->last;
        if(ptr1) os_free(ptr1,sizeof(struct strace_info));
        os_free(st_base,sizeof(struct strace_head));
        current->st_md_base=NULL;
        return 0;

}



///////////////////////////////////////////////////////////////////////////
//// 		Start of ftrace functionality 		      	      /////
///////////////////////////////////////////////////////////////////////////

int exist(struct ftrace_head* ft_md_base,unsigned long faddr){
        if(ft_md_base->next==NULL) return 0;
        struct ftrace_info* temp=ft_md_base->next;
        while(1){
			if(!temp) break;
                if(temp->faddr==faddr){
                        return 1;
                }
                temp=temp->next;
        }
        return 0;
}

long do_ftrace(struct exec_context *ctx, unsigned long faddr, long action, long nargs, int fd_trace_buffer)
{
    if(ctx->ft_md_base==NULL){
                if(action!=ADD_FTRACE) return -EINVAL;
                struct ftrace_head *ft_md_base=(struct ftrace_head *)os_alloc(sizeof(struct ftrace_head));
                ft_md_base->count=1;
                struct ftrace_info* info=(struct ftrace_info*)os_alloc(sizeof(struct ftrace_info));
                info->faddr=faddr;
                info->num_args=nargs;
                info->fd=fd_trace_buffer;
                info->next=NULL;
                info->capture_backtrace=0;
                ft_md_base->next=info;
                ft_md_base->last=info;
                ctx->ft_md_base=ft_md_base;
                return 0;
        }
        if(action==ADD_FTRACE){
                if(exist(ctx->ft_md_base,faddr)==1) return -EINVAL;
                if(ctx->ft_md_base->count==FTRACE_MAX) return -EINVAL;
                struct ftrace_info* info=(struct ftrace_info*)os_alloc(sizeof(struct ftrace_info));
                info->faddr=faddr;
                info->num_args=nargs;
                info->fd=fd_trace_buffer;
                info->next=NULL;
                info->capture_backtrace=0;
                struct ftrace_info* temp=ctx->ft_md_base->next;
                if(temp==NULL){
                        ctx->ft_md_base->next=info;
                        ctx->ft_md_base->last=info;
                        return 0;
                }
                while(temp->next){
                        temp=temp->next;
                }
                temp->next=info;
                ctx->ft_md_base->last=info;
                return 0;
        }
        if(action==REMOVE_FTRACE){
                if(exist(ctx->ft_md_base,faddr)==0) return -EINVAL;
                struct ftrace_info* temp=ctx->ft_md_base->next->next;
                struct ftrace_info* prev=ctx->ft_md_base->next;
                if(prev->faddr==faddr){
                        ctx->ft_md_base->next=temp;
                        if(temp==NULL) ctx->ft_md_base->last=NULL;
                        os_free(prev,sizeof(struct ftrace_info));
                        return 0;
                }
                while(temp){
                        if(faddr == temp->faddr){
                                if(ctx->ft_md_base->last == temp){
                                        ctx->ft_md_base->last=prev;
                                }
                                prev->next=temp->next;
                                os_free(temp,sizeof(struct ftrace_info));
                                return 0;
                        }
                        temp=temp->next;
                        prev=prev->next;
                }
                return -EINVAL;
        }
        if(ENABLE_FTRACE == action){
                if(exist(ctx->ft_md_base,faddr)==0)
                        {
                                printk("NOT found\n"); return -EINVAL;}
                struct ftrace_info* temp=ctx->ft_md_base->next;
                while(temp){
                        if(faddr == temp->faddr){
                                if(*((u8 *)faddr+1)==INV_OPCODE) return 0;
                                for(int i=0; i<4; i++){
                                        temp->code_backup[i]=*((u8 *)faddr+i);
                                }
                                for(int i=0; i<4; i++){
                                        *((u8 *)faddr+i)=INV_OPCODE;
                                }
                                return 0;
                        }
                        temp=temp->next;
                }
                return 0;
        }
        if(action==DISABLE_FTRACE){
                if(exist(ctx->ft_md_base,faddr)==0) return -EINVAL;
                struct ftrace_info* temp=ctx->ft_md_base->next;
                while(temp){
                        if(temp->faddr==faddr){
                                if(*((u8 *)faddr+1)!=INV_OPCODE) return 0;
                                for(int i=0; i<4; i++){
                                        *((u8 *)faddr+i)=temp->code_backup[i];
                                }
                                return 0;
                        }
                        temp=temp->next;
                }


                return 0;
        }
        return 0;

}

//Fault handler
long handle_ftrace_fault(struct user_regs *regs)
{
        return 0;
}


int sys_read_ftrace(struct file *filep, char *buff, u64 count)
{
    return 0;
}


