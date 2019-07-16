; Copyright (c) 2019, Ryo Currency Project
;
; Portions of this file are available under BSD-3 license. Please see ORIGINAL-LICENSE for details
; All rights reserved.
;
; Authors and copyright holders give permission for following:
;
; 1. Redistribution and use in source and binary forms WITHOUT modification.
;
; 2. Modification of the source form for your own personal use.
;
; As long as the following conditions are met:
;
; 3. You must not distribute modified copies of the work to third parties. This includes
;    posting the work online, or hosting copies of the modified work for download.
;
; 4. Any derivative version of this work is also covered by this license, including point 8.
;
; 5. Neither the name of the copyright holders nor the names of the authors may be
;    used to endorse or promote products derived from this software without specific
;    prior written permission.
;
; 6. You agree that this licence is governed by and shall be construed in accordance
;    with the laws of England and Wales.
;
; 7. You agree to submit all disputes arising out of or in connection with this licence
;    to the exclusive jurisdiction of the Courts of England and Wales.
;
; Authors and copyright holders agree that:
;
; 8. This licence expires and the work covered by it is released into the
;    public domain on 1st of February 2020
;
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
; EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
; MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
; THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
; SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
; INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
; STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
; THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


; Based on Dr Bernstein's public domain code.
; https://bench.cr.yp.to/supercop.html

DEFAULT REL

section .data
const_38 dq 38
const_EC2D0 dq 0xEBD69B9426B2F146
const_EC2D1 dq 0x00E0149A8283B156
const_EC2D2 dq 0x198E80F2EEF3D130
const_EC2D3 dq 0xA406D9DC56DFFCE7

section .text
global sV_fe64_mul
global sV_fe64_square
global sV_fe64_add
global sV_fe64_sub
global sV_fe64_reduce
global sV_ge64_add
global sV_ge64_p1p1_to_p3
global sV_ge64_p1p1_to_p2
global sV_ge64_nielsadd2
global sV_ge64_pnielsadd_p1p1
global sV_ge64_p2_dbl
global win64_wrapper

win64_wrapper:
sub    rsp,0x10
mov    QWORD [rsp+0x00],rdi
mov    QWORD [rsp+0x08],rsi
mov    rdi,rdx
mov    rsi,r8
mov    rdx,r9
call   rcx
mov    rdi,QWORD [rsp+0x00]
mov    rsi,QWORD [rsp+0x08]
add    rsp,0x10
ret

sV_fe64_mul:
mov    r11,rsp
and    r11,0x1f
add    r11,0x40
sub    rsp,r11
mov    QWORD [rsp],r11
mov    QWORD [rsp+0x8],r12
mov    QWORD [rsp+0x10],r13
mov    QWORD [rsp+0x18],r14
mov    QWORD [rsp+0x20],r15
mov    QWORD [rsp+0x28],rbx
mov    QWORD [rsp+0x30],rbp
mov    rcx,rdx
mov    r8,0x0
mov    r9,0x0
mov    r10,0x0
mov    r11,0x0
mov    r12,QWORD [rsi]
mov    rax,QWORD [rcx]
mul    r12
mov    r13,rax
mov    r14,rdx
mov    rax,QWORD [rcx+0x8]
mul    r12
add    r14,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,QWORD [rcx+0x10]
mul    r12
add    r15,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rcx+0x18]
mul    r12
add    rbx,rax
adc    r8,rdx
mov    r12,QWORD [rsi+0x8]
mov    rax,QWORD [rcx]
mul    r12
add    r14,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x8]
mul    r12
add    r15,rax
adc    rdx,0x0
add    r15,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x10]
mul    r12
add    rbx,rax
adc    rdx,0x0
add    rbx,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x18]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
adc    r9,rdx
mov    r12,QWORD [rsi+0x10]
mov    rax,QWORD [rcx]
mul    r12
add    r15,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x8]
mul    r12
add    rbx,rax
adc    rdx,0x0
add    rbx,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x10]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x18]
mul    r12
add    r9,rax
adc    rdx,0x0
add    r9,rbp
adc    r10,rdx
mov    rsi,QWORD [rsi+0x18]
mov    rax,QWORD [rcx]
mul    rsi
add    rbx,rax
mov    r12,0x0
adc    r12,rdx
mov    rax,QWORD [rcx+0x8]
mul    rsi
add    r8,rax
adc    rdx,0x0
add    r8,r12
mov    r12,0x0
adc    r12,rdx
mov    rax,QWORD [rcx+0x10]
mul    rsi
add    r9,rax
adc    rdx,0x0
add    r9,r12
mov    r12,0x0
adc    r12,rdx
mov    rax,QWORD [rcx+0x18]
mul    rsi
add    r10,rax
adc    rdx,0x0
add    r10,r12
adc    r11,rdx
mov    rax,r8
mul    QWORD [const_38]
mov    rsi,rax
mov    rax,r9
mov    rcx,rdx
mul    QWORD [const_38]
add    rcx,rax
mov    rax,r10
mov    r8,0x0
adc    r8,rdx
mul    QWORD [const_38]
add    r8,rax
mov    rax,r11
mov    r9,0x0
adc    r9,rdx
mul    QWORD [const_38]
add    r9,rax
mov    rax,0x0
adc    rax,rdx
add    r13,rsi
adc    r14,rcx
adc    r15,r8
adc    rbx,r9
mov    rsi,0x0
adc    rax,rsi
imul   rdx,rax,0x26
add    r13,rdx
adc    r14,rsi
adc    r15,rsi
adc    rbx,rsi
adc    rsi,rsi
imul   rsi,rsi,0x26
add    r13,rsi
mov    QWORD [rdi+0x8],r14
mov    QWORD [rdi+0x10],r15
mov    QWORD [rdi+0x18],rbx
mov    QWORD [rdi],r13
mov    r11,QWORD [rsp]
mov    r12,QWORD [rsp+0x8]
mov    r13,QWORD [rsp+0x10]
mov    r14,QWORD [rsp+0x18]
mov    r15,QWORD [rsp+0x20]
mov    rbx,QWORD [rsp+0x28]
mov    rbp,QWORD [rsp+0x30]
add    rsp,r11
mov    rax,rdi
mov    rdx,rsi
ret

sV_fe64_square:
mov    r11,rsp
and    r11,0x1f
add    r11,0x40
sub    rsp,r11
mov    QWORD [rsp],r11
mov    QWORD [rsp+0x8],r12
mov    QWORD [rsp+0x10],r13
mov    QWORD [rsp+0x18],r14
mov    QWORD [rsp+0x20],r15
mov    QWORD [rsp+0x28],rbx
mov    QWORD [rsp+0x30],rbp
mov    rcx,0x0
mov    rax,QWORD [rsi+0x8]
mul    QWORD [rsi]
mov    r8,rax
mov    r9,rdx
mov    rax,QWORD [rsi+0x10]
mul    QWORD [rsi+0x8]
mov    r10,rax
mov    r11,rdx
mov    rax,QWORD [rsi+0x18]
mul    QWORD [rsi+0x10]
mov    r12,rax
mov    r13,rdx
mov    rax,QWORD [rsi+0x10]
mul    QWORD [rsi]
add    r9,rax
adc    r10,rdx
adc    r11,0x0
mov    rax,QWORD [rsi+0x18]
mul    QWORD [rsi+0x8]
add    r11,rax
adc    r12,rdx
adc    r13,0x0
mov    rax,QWORD [rsi+0x18]
mul    QWORD [rsi]
add    r10,rax
adc    r11,rdx
adc    r12,0x0
adc    r13,0x0
adc    rcx,0x0
add    r8,r8
adc    r9,r9
adc    r10,r10
adc    r11,r11
adc    r12,r12
adc    r13,r13
adc    rcx,rcx
mov    rax,QWORD [rsi]
mul    QWORD [rsi]
mov    r14,rax
mov    r15,rdx
mov    rax,QWORD [rsi+0x8]
mul    QWORD [rsi+0x8]
mov    rbx,rax
mov    rbp,rdx
mov    rax,QWORD [rsi+0x10]
mul    QWORD [rsi+0x10]
add    r8,r15
adc    r9,rbx
adc    r10,rbp
adc    r11,rax
adc    r12,rdx
adc    r13,0x0
adc    rcx,0x0
mov    rax,QWORD [rsi+0x18]
mul    QWORD [rsi+0x18]
add    r13,rax
adc    rcx,rdx
mov    rax,r11
mul    QWORD [const_38]
mov    rsi,rax
mov    rax,r12
mov    r11,rdx
mul    QWORD [const_38]
add    r11,rax
mov    rax,r13
mov    r12,0x0
adc    r12,rdx
mul    QWORD [const_38]
add    r12,rax
mov    rax,rcx
mov    rcx,0x0
adc    rcx,rdx
mul    QWORD [const_38]
add    rcx,rax
mov    rax,0x0
adc    rax,rdx
add    r14,rsi
adc    r8,r11
adc    r9,r12
adc    r10,rcx
mov    rsi,0x0
adc    rax,rsi
imul   rdx,rax,0x26
add    r14,rdx
adc    r8,rsi
adc    r9,rsi
adc    r10,rsi
adc    rsi,rsi
imul   rsi,rsi,0x26
add    r14,rsi
mov    QWORD [rdi+0x8],r8
mov    QWORD [rdi+0x10],r9
mov    QWORD [rdi+0x18],r10
mov    QWORD [rdi],r14
mov    r11,QWORD [rsp]
mov    r12,QWORD [rsp+0x8]
mov    r13,QWORD [rsp+0x10]
mov    r14,QWORD [rsp+0x18]
mov    r15,QWORD [rsp+0x20]
mov    rbx,QWORD [rsp+0x28]
mov    rbp,QWORD [rsp+0x30]
add    rsp,r11
mov    rax,rdi
mov    rdx,rsi
ret

sV_fe64_add:
mov    r11,rsp
and    r11,0x1f
add    r11,0x0
sub    rsp,r11
mov    rcx,QWORD [rsi]
mov    r8,QWORD [rsi+0x8]
mov    r9,QWORD [rsi+0x10]
mov    rsi,QWORD [rsi+0x18]
add    rcx,QWORD [rdx]
adc    r8,QWORD [rdx+0x8]
adc    r9,QWORD [rdx+0x10]
adc    rsi,QWORD [rdx+0x18]
mov    rdx,0x0
mov    rax,0x26
cmovae rax,rdx
add    rcx,rax
adc    r8,rdx
adc    r9,rdx
adc    rsi,rdx
cmovb  rdx,rax
add    rcx,rdx
mov    QWORD [rdi],rcx
mov    QWORD [rdi+0x8],r8
mov    QWORD [rdi+0x10],r9
mov    QWORD [rdi+0x18],rsi
add    rsp,r11
mov    rax,rdi
mov    rdx,rsi
ret

sV_fe64_sub:
mov    r11,rsp
and    r11,0x1f
add    r11,0x0
sub    rsp,r11
mov    rcx,QWORD [rsi]
mov    r8,QWORD [rsi+0x8]
mov    r9,QWORD [rsi+0x10]
mov    rsi,QWORD [rsi+0x18]
sub    rcx,QWORD [rdx]
sbb    r8,QWORD [rdx+0x8]
sbb    r9,QWORD [rdx+0x10]
sbb    rsi,QWORD [rdx+0x18]
mov    rdx,0x0
mov    rax,0x26
cmovae rax,rdx
sub    rcx,rax
sbb    r8,rdx
sbb    r9,rdx
sbb    rsi,rdx
cmovb  rdx,rax
sub    rcx,rdx
mov    QWORD [rdi],rcx
mov    QWORD [rdi+0x8],r8
mov    QWORD [rdi+0x10],r9
mov    QWORD [rdi+0x18],rsi
add    rsp,r11
mov    rax,rdi
mov    rdx,rsi
ret

sV_fe64_reduce:
mov    r11,rsp
and    r11,0x1f
add    r11,0x40
sub    rsp,r11
mov    QWORD [rsp],r11
mov    QWORD [rsp+0x8],r12
mov    QWORD [rsp+0x10],r13
mov    QWORD [rsp+0x18],r14
mov    QWORD [rsp+0x20],r15
mov    QWORD [rsp+0x28],rbx
mov    QWORD [rsp+0x30],rbp
mov    rsi,QWORD [rdi]
mov    rdx,QWORD [rdi+0x8]
mov    rcx,QWORD [rdi+0x10]
mov    r8,QWORD [rdi+0x18]
mov    r9,rsi
mov    rax,rdx
mov    r10,rcx
mov    r11,r8
mov    r12,0x1
shl    r12,0x3f
add    r9,0x13
adc    rax,0x0
adc    r10,0x0
adc    r11,r12
cmovb  rsi,r9
cmovb  rdx,rax
cmovb  rcx,r10
cmovb  r8,r11
mov    r9,rsi
mov    rax,rdx
mov    r10,rcx
mov    r11,r8
add    r9,0x13
adc    rax,0x0
adc    r10,0x0
adc    r11,r12
cmovb  rsi,r9
cmovb  rdx,rax
cmovb  rcx,r10
cmovb  r8,r11
mov    QWORD [rdi],rsi
mov    QWORD [rdi+0x8],rdx
mov    QWORD [rdi+0x10],rcx
mov    QWORD [rdi+0x18],r8
mov    r11,QWORD [rsp]
mov    r12,QWORD [rsp+0x8]
mov    r13,QWORD [rsp+0x10]
mov    r14,QWORD [rsp+0x18]
mov    r15,QWORD [rsp+0x20]
mov    rbx,QWORD [rsp+0x28]
mov    rbp,QWORD [rsp+0x30]
add    rsp,r11
mov    rax,rdi
mov    rdx,rsi
ret

sV_ge64_add:
mov    r11,rsp
and    r11,0x1f
add    r11,0xc0
sub    rsp,r11
mov    QWORD [rsp],r11
mov    QWORD [rsp+0x8],r12
mov    QWORD [rsp+0x10],r13
mov    QWORD [rsp+0x18],r14
mov    QWORD [rsp+0x20],r15
mov    QWORD [rsp+0x28],rbx
mov    QWORD [rsp+0x30],rbp
mov    rcx,rdx
mov    rdx,QWORD [rsi+0x20]
mov    r8,QWORD [rsi+0x28]
mov    r9,QWORD [rsi+0x30]
mov    rax,QWORD [rsi+0x38]
mov    r10,rdx
mov    r11,r8
mov    r12,r9
mov    r13,rax
sub    rdx,QWORD [rsi]
sbb    r8,QWORD [rsi+0x8]
sbb    r9,QWORD [rsi+0x10]
sbb    rax,QWORD [rsi+0x18]
mov    r14,0x0
mov    r15,0x26
cmovae r15,r14
sub    rdx,r15
sbb    r8,r14
sbb    r9,r14
sbb    rax,r14
cmovb  r14,r15
sub    rdx,r14
add    r10,QWORD [rsi]
adc    r11,QWORD [rsi+0x8]
adc    r12,QWORD [rsi+0x10]
adc    r13,QWORD [rsi+0x18]
mov    r14,0x0
mov    r15,0x26
cmovae r15,r14
add    r10,r15
adc    r11,r14
adc    r12,r14
adc    r13,r14
cmovb  r14,r15
add    r10,r14
mov    QWORD [rsp+0x38],rdx
mov    QWORD [rsp+0x40],r8
mov    QWORD [rsp+0x48],r9
mov    QWORD [rsp+0x50],rax
mov    QWORD [rsp+0x58],r10
mov    QWORD [rsp+0x60],r11
mov    QWORD [rsp+0x68],r12
mov    QWORD [rsp+0x70],r13
mov    rdx,QWORD [rcx+0x20]
mov    r8,QWORD [rcx+0x28]
mov    r9,QWORD [rcx+0x30]
mov    rax,QWORD [rcx+0x38]
mov    r10,rdx
mov    r11,r8
mov    r12,r9
mov    r13,rax
sub    rdx,QWORD [rcx]
sbb    r8,QWORD [rcx+0x8]
sbb    r9,QWORD [rcx+0x10]
sbb    rax,QWORD [rcx+0x18]
mov    r14,0x0
mov    r15,0x26
cmovae r15,r14
sub    rdx,r15
sbb    r8,r14
sbb    r9,r14
sbb    rax,r14
cmovb  r14,r15
sub    rdx,r14
add    r10,QWORD [rcx]
adc    r11,QWORD [rcx+0x8]
adc    r12,QWORD [rcx+0x10]
adc    r13,QWORD [rcx+0x18]
mov    r14,0x0
mov    r15,0x26
cmovae r15,r14
add    r10,r15
adc    r11,r14
adc    r12,r14
adc    r13,r14
cmovb  r14,r15
add    r10,r14
mov    QWORD [rsp+0x78],rdx
mov    QWORD [rsp+0x80],r8
mov    QWORD [rsp+0x88],r9
mov    QWORD [rsp+0x90],rax
mov    QWORD [rsp+0x98],r10
mov    QWORD [rsp+0xa0],r11
mov    QWORD [rsp+0xa8],r12
mov    QWORD [rsp+0xb0],r13
mov    r8,0x0
mov    r9,0x0
mov    r10,0x0
mov    r11,0x0
mov    r12,QWORD [rsp+0x38]
mov    rax,QWORD [rsp+0x78]
mul    r12
mov    r13,rax
mov    r14,rdx
mov    rax,QWORD [rsp+0x80]
mul    r12
add    r14,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,QWORD [rsp+0x88]
mul    r12
add    r15,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsp+0x90]
mul    r12
add    rbx,rax
adc    r8,rdx
mov    r12,QWORD [rsp+0x40]
mov    rax,QWORD [rsp+0x78]
mul    r12
add    r14,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rsp+0x80]
mul    r12
add    r15,rax
adc    rdx,0x0
add    r15,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rsp+0x88]
mul    r12
add    rbx,rax
adc    rdx,0x0
add    rbx,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rsp+0x90]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
adc    r9,rdx
mov    r12,QWORD [rsp+0x48]
mov    rax,QWORD [rsp+0x78]
mul    r12
add    r15,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rsp+0x80]
mul    r12
add    rbx,rax
adc    rdx,0x0
add    rbx,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rsp+0x88]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rsp+0x90]
mul    r12
add    r9,rax
adc    rdx,0x0
add    r9,rbp
adc    r10,rdx
mov    r12,QWORD [rsp+0x50]
mov    rax,QWORD [rsp+0x78]
mul    r12
add    rbx,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rsp+0x80]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rsp+0x88]
mul    r12
add    r9,rax
adc    rdx,0x0
add    r9,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rsp+0x90]
mul    r12
add    r10,rax
adc    rdx,0x0
add    r10,rbp
adc    r11,rdx
mov    rax,r8
mul    QWORD [const_38]
mov    r8,rax
mov    rax,r9
mov    r9,rdx
mul    QWORD [const_38]
add    r9,rax
mov    rax,r10
mov    r10,0x0
adc    r10,rdx
mul    QWORD [const_38]
add    r10,rax
mov    rax,r11
mov    r11,0x0
adc    r11,rdx
mul    QWORD [const_38]
add    r11,rax
mov    rax,0x0
adc    rax,rdx
add    r13,r8
adc    r14,r9
adc    r15,r10
adc    rbx,r11
mov    rdx,0x0
adc    rax,rdx
imul   r8,rax,0x26
add    r13,r8
adc    r14,rdx
adc    r15,rdx
adc    rbx,rdx
adc    rdx,rdx
imul   rdx,rdx,0x26
add    r13,rdx
mov    QWORD [rsp+0x38],r13
mov    QWORD [rsp+0x40],r14
mov    QWORD [rsp+0x48],r15
mov    QWORD [rsp+0x50],rbx
mov    r8,0x0
mov    r9,0x0
mov    r10,0x0
mov    r11,0x0
mov    r12,QWORD [rsp+0x58]
mov    rax,QWORD [rsp+0x98]
mul    r12
mov    r13,rax
mov    r14,rdx
mov    rax,QWORD [rsp+0xa0]
mul    r12
add    r14,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,QWORD [rsp+0xa8]
mul    r12
add    r15,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsp+0xb0]
mul    r12
add    rbx,rax
adc    r8,rdx
mov    r12,QWORD [rsp+0x60]
mov    rax,QWORD [rsp+0x98]
mul    r12
add    r14,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rsp+0xa0]
mul    r12
add    r15,rax
adc    rdx,0x0
add    r15,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rsp+0xa8]
mul    r12
add    rbx,rax
adc    rdx,0x0
add    rbx,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rsp+0xb0]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
adc    r9,rdx
mov    r12,QWORD [rsp+0x68]
mov    rax,QWORD [rsp+0x98]
mul    r12
add    r15,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rsp+0xa0]
mul    r12
add    rbx,rax
adc    rdx,0x0
add    rbx,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rsp+0xa8]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rsp+0xb0]
mul    r12
add    r9,rax
adc    rdx,0x0
add    r9,rbp
adc    r10,rdx
mov    r12,QWORD [rsp+0x70]
mov    rax,QWORD [rsp+0x98]
mul    r12
add    rbx,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rsp+0xa0]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rsp+0xa8]
mul    r12
add    r9,rax
adc    rdx,0x0
add    r9,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rsp+0xb0]
mul    r12
add    r10,rax
adc    rdx,0x0
add    r10,rbp
adc    r11,rdx
mov    rax,r8
mul    QWORD [const_38]
mov    r8,rax
mov    rax,r9
mov    r9,rdx
mul    QWORD [const_38]
add    r9,rax
mov    rax,r10
mov    r10,0x0
adc    r10,rdx
mul    QWORD [const_38]
add    r10,rax
mov    rax,r11
mov    r11,0x0
adc    r11,rdx
mul    QWORD [const_38]
add    r11,rax
mov    rax,0x0
adc    rax,rdx
add    r13,r8
adc    r14,r9
adc    r15,r10
adc    rbx,r11
mov    rdx,0x0
adc    rax,rdx
imul   r8,rax,0x26
add    r13,r8
adc    r14,rdx
adc    r15,rdx
adc    rbx,rdx
adc    rdx,rdx
imul   rdx,rdx,0x26
add    r13,rdx
mov    rdx,r13
mov    r8,r14
mov    r9,r15
mov    rax,rbx
add    rdx,QWORD [rsp+0x38]
adc    r8,QWORD [rsp+0x40]
adc    r9,QWORD [rsp+0x48]
adc    rax,QWORD [rsp+0x50]
mov    r10,0x0
mov    r11,0x26
cmovae r11,r10
add    rdx,r11
adc    r8,r10
adc    r9,r10
adc    rax,r10
cmovb  r10,r11
add    rdx,r10
sub    r13,QWORD [rsp+0x38]
sbb    r14,QWORD [rsp+0x40]
sbb    r15,QWORD [rsp+0x48]
sbb    rbx,QWORD [rsp+0x50]
mov    r10,0x0
mov    r11,0x26
cmovae r11,r10
sub    r13,r11
sbb    r14,r10
sbb    r15,r10
sbb    rbx,r10
cmovb  r10,r11
sub    r13,r10
mov    QWORD [rdi],r13
mov    QWORD [rdi+0x8],r14
mov    QWORD [rdi+0x10],r15
mov    QWORD [rdi+0x18],rbx
mov    QWORD [rdi+0x40],rdx
mov    QWORD [rdi+0x48],r8
mov    QWORD [rdi+0x50],r9
mov    QWORD [rdi+0x58],rax
mov    r8,0x0
mov    r9,0x0
mov    r10,0x0
mov    r11,0x0
mov    r12,QWORD [rsi+0x60]
mov    rax,QWORD [rcx+0x60]
mul    r12
mov    r13,rax
mov    r14,rdx
mov    rax,QWORD [rcx+0x68]
mul    r12
add    r14,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,QWORD [rcx+0x70]
mul    r12
add    r15,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rcx+0x78]
mul    r12
add    rbx,rax
adc    r8,rdx
mov    r12,QWORD [rsi+0x68]
mov    rax,QWORD [rcx+0x60]
mul    r12
add    r14,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x68]
mul    r12
add    r15,rax
adc    rdx,0x0
add    r15,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x70]
mul    r12
add    rbx,rax
adc    rdx,0x0
add    rbx,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x78]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
adc    r9,rdx
mov    r12,QWORD [rsi+0x70]
mov    rax,QWORD [rcx+0x60]
mul    r12
add    r15,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x68]
mul    r12
add    rbx,rax
adc    rdx,0x0
add    rbx,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x70]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x78]
mul    r12
add    r9,rax
adc    rdx,0x0
add    r9,rbp
adc    r10,rdx
mov    r12,QWORD [rsi+0x78]
mov    rax,QWORD [rcx+0x60]
mul    r12
add    rbx,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x68]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x70]
mul    r12
add    r9,rax
adc    rdx,0x0
add    r9,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x78]
mul    r12
add    r10,rax
adc    rdx,0x0
add    r10,rbp
adc    r11,rdx
mov    rax,r8
mul    QWORD [const_38]
mov    r8,rax
mov    rax,r9
mov    r9,rdx
mul    QWORD [const_38]
add    r9,rax
mov    rax,r10
mov    r10,0x0
adc    r10,rdx
mul    QWORD [const_38]
add    r10,rax
mov    rax,r11
mov    r11,0x0
adc    r11,rdx
mul    QWORD [const_38]
add    r11,rax
mov    rax,0x0
adc    rax,rdx
add    r13,r8
adc    r14,r9
adc    r15,r10
adc    rbx,r11
mov    rdx,0x0
adc    rax,rdx
imul   r8,rax,0x26
add    r13,r8
adc    r14,rdx
adc    r15,rdx
adc    rbx,rdx
adc    rdx,rdx
imul   rdx,rdx,0x26
add    r13,rdx
mov    QWORD [rsp+0x38],r13
mov    QWORD [rsp+0x40],r14
mov    QWORD [rsp+0x48],r15
mov    QWORD [rsp+0x50],rbx
mov    r8,0x0
mov    r9,0x0
mov    r10,0x0
mov    r11,0x0
mov    r12,QWORD [rsp+0x38]
mov    rax,QWORD [const_EC2D0]
mul    r12
mov    r13,rax
mov    r14,rdx
mov    rax,QWORD [const_EC2D1]
mul    r12
add    r14,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,QWORD [const_EC2D2]
mul    r12
add    r15,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [const_EC2D3]
mul    r12
add    rbx,rax
adc    r8,rdx
mov    r12,QWORD [rsp+0x40]
mov    rax,QWORD [const_EC2D0]
mul    r12
add    r14,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [const_EC2D1]
mul    r12
add    r15,rax
adc    rdx,0x0
add    r15,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [const_EC2D2]
mul    r12
add    rbx,rax
adc    rdx,0x0
add    rbx,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [const_EC2D3]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
adc    r9,rdx
mov    r12,QWORD [rsp+0x48]
mov    rax,QWORD [const_EC2D0]
mul    r12
add    r15,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [const_EC2D1]
mul    r12
add    rbx,rax
adc    rdx,0x0
add    rbx,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [const_EC2D2]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [const_EC2D3]
mul    r12
add    r9,rax
adc    rdx,0x0
add    r9,rbp
adc    r10,rdx
mov    r12,QWORD [rsp+0x50]
mov    rax,QWORD [const_EC2D0]
mul    r12
add    rbx,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [const_EC2D1]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [const_EC2D2]
mul    r12
add    r9,rax
adc    rdx,0x0
add    r9,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [const_EC2D3]
mul    r12
add    r10,rax
adc    rdx,0x0
add    r10,rbp
adc    r11,rdx
mov    rax,r8
mul    QWORD [const_38]
mov    r8,rax
mov    rax,r9
mov    r9,rdx
mul    QWORD [const_38]
add    r9,rax
mov    rax,r10
mov    r10,0x0
adc    r10,rdx
mul    QWORD [const_38]
add    r10,rax
mov    rax,r11
mov    r11,0x0
adc    r11,rdx
mul    QWORD [const_38]
add    r11,rax
mov    rax,0x0
adc    rax,rdx
add    r13,r8
adc    r14,r9
adc    r15,r10
adc    rbx,r11
mov    rdx,0x0
adc    rax,rdx
imul   r8,rax,0x26
add    r13,r8
adc    r14,rdx
adc    r15,rdx
adc    rbx,rdx
adc    rdx,rdx
imul   rdx,rdx,0x26
add    r13,rdx
mov    QWORD [rsp+0x38],r13
mov    QWORD [rsp+0x40],r14
mov    QWORD [rsp+0x48],r15
mov    QWORD [rsp+0x50],rbx
mov    r8,0x0
mov    r9,0x0
mov    r10,0x0
mov    r11,0x0
mov    r12,QWORD [rsi+0x40]
mov    rax,QWORD [rcx+0x40]
mul    r12
mov    r13,rax
mov    r14,rdx
mov    rax,QWORD [rcx+0x48]
mul    r12
add    r14,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,QWORD [rcx+0x50]
mul    r12
add    r15,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rcx+0x58]
mul    r12
add    rbx,rax
adc    r8,rdx
mov    r12,QWORD [rsi+0x48]
mov    rax,QWORD [rcx+0x40]
mul    r12
add    r14,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x48]
mul    r12
add    r15,rax
adc    rdx,0x0
add    r15,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x50]
mul    r12
add    rbx,rax
adc    rdx,0x0
add    rbx,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x58]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
adc    r9,rdx
mov    r12,QWORD [rsi+0x50]
mov    rax,QWORD [rcx+0x40]
mul    r12
add    r15,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x48]
mul    r12
add    rbx,rax
adc    rdx,0x0
add    rbx,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x50]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x58]
mul    r12
add    r9,rax
adc    rdx,0x0
add    r9,rbp
adc    r10,rdx
mov    rsi,QWORD [rsi+0x58]
mov    rax,QWORD [rcx+0x40]
mul    rsi
add    rbx,rax
mov    r12,0x0
adc    r12,rdx
mov    rax,QWORD [rcx+0x48]
mul    rsi
add    r8,rax
adc    rdx,0x0
add    r8,r12
mov    r12,0x0
adc    r12,rdx
mov    rax,QWORD [rcx+0x50]
mul    rsi
add    r9,rax
adc    rdx,0x0
add    r9,r12
mov    r12,0x0
adc    r12,rdx
mov    rax,QWORD [rcx+0x58]
mul    rsi
add    r10,rax
adc    rdx,0x0
add    r10,r12
adc    r11,rdx
mov    rax,r8
mul    QWORD [const_38]
mov    rsi,rax
mov    rax,r9
mov    rcx,rdx
mul    QWORD [const_38]
add    rcx,rax
mov    rax,r10
mov    r8,0x0
adc    r8,rdx
mul    QWORD [const_38]
add    r8,rax
mov    rax,r11
mov    r9,0x0
adc    r9,rdx
mul    QWORD [const_38]
add    r9,rax
mov    rax,0x0
adc    rax,rdx
add    r13,rsi
adc    r14,rcx
adc    r15,r8
adc    rbx,r9
mov    rsi,0x0
adc    rax,rsi
imul   rdx,rax,0x26
add    r13,rdx
adc    r14,rsi
adc    r15,rsi
adc    rbx,rsi
adc    rsi,rsi
imul   rsi,rsi,0x26
add    r13,rsi
add    r13,r13
adc    r14,r14
adc    r15,r15
adc    rbx,rbx
mov    rsi,0x0
mov    rdx,0x26
cmovae rdx,rsi
add    r13,rdx
adc    r14,rsi
adc    r15,rsi
adc    rbx,rsi
cmovb  rsi,rdx
add    r13,rsi
mov    rsi,r13
mov    rdx,r14
mov    rcx,r15
mov    r8,rbx
add    rsi,QWORD [rsp+0x38]
adc    rdx,QWORD [rsp+0x40]
adc    rcx,QWORD [rsp+0x48]
adc    r8,QWORD [rsp+0x50]
mov    r9,0x0
mov    rax,0x26
cmovae rax,r9
add    rsi,rax
adc    rdx,r9
adc    rcx,r9
adc    r8,r9
cmovb  r9,rax
add    rsi,r9
sub    r13,QWORD [rsp+0x38]
sbb    r14,QWORD [rsp+0x40]
sbb    r15,QWORD [rsp+0x48]
sbb    rbx,QWORD [rsp+0x50]
mov    r9,0x0
mov    rax,0x26
cmovae rax,r9
sub    r13,rax
sbb    r14,r9
sbb    r15,r9
sbb    rbx,r9
cmovb  r9,rax
sub    r13,r9
mov    QWORD [rdi+0x20],rsi
mov    QWORD [rdi+0x28],rdx
mov    QWORD [rdi+0x30],rcx
mov    QWORD [rdi+0x38],r8
mov    QWORD [rdi+0x60],r13
mov    QWORD [rdi+0x68],r14
mov    QWORD [rdi+0x70],r15
mov    QWORD [rdi+0x78],rbx
mov    r11,QWORD [rsp]
mov    r12,QWORD [rsp+0x8]
mov    r13,QWORD [rsp+0x10]
mov    r14,QWORD [rsp+0x18]
mov    r15,QWORD [rsp+0x20]
mov    rbx,QWORD [rsp+0x28]
mov    rbp,QWORD [rsp+0x30]
add    rsp,r11
mov    rax,rdi
mov    rdx,rsi
ret

sV_ge64_p1p1_to_p3:
mov    r11,rsp
and    r11,0x1f
add    r11,0x40
sub    rsp,r11
mov    QWORD [rsp],r11
mov    QWORD [rsp+0x8],r12
mov    QWORD [rsp+0x10],r13
mov    QWORD [rsp+0x18],r14
mov    QWORD [rsp+0x20],r15
mov    QWORD [rsp+0x28],rbx
mov    QWORD [rsp+0x30],rbp
mov    rcx,0x0
mov    r8,0x0
mov    r9,0x0
mov    r10,0x0
mov    r11,QWORD [rsi]
mov    rax,QWORD [rsi+0x60]
mul    r11
mov    r12,rax
mov    r13,rdx
mov    rax,QWORD [rsi+0x68]
mul    r11
add    r13,rax
mov    r14,0x0
adc    r14,rdx
mov    rax,QWORD [rsi+0x70]
mul    r11
add    r14,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,QWORD [rsi+0x78]
mul    r11
add    r15,rax
adc    rcx,rdx
mov    r11,QWORD [rsi+0x8]
mov    rax,QWORD [rsi+0x60]
mul    r11
add    r13,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x68]
mul    r11
add    r14,rax
adc    rdx,0x0
add    r14,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x70]
mul    r11
add    r15,rax
adc    rdx,0x0
add    r15,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x78]
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
adc    r8,rdx
mov    r11,QWORD [rsi+0x10]
mov    rax,QWORD [rsi+0x60]
mul    r11
add    r14,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x68]
mul    r11
add    r15,rax
adc    rdx,0x0
add    r15,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x70]
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x78]
mul    r11
add    r8,rax
adc    rdx,0x0
add    r8,rbx
adc    r9,rdx
mov    r11,QWORD [rsi+0x18]
mov    rax,QWORD [rsi+0x60]
mul    r11
add    r15,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x68]
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x70]
mul    r11
add    r8,rax
adc    rdx,0x0
add    r8,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x78]
mul    r11
add    r9,rax
adc    rdx,0x0
add    r9,rbx
adc    r10,rdx
mov    rax,rcx
mul    QWORD [const_38]
mov    rcx,rax
mov    rax,r8
mov    r8,rdx
mul    QWORD [const_38]
add    r8,rax
mov    rax,r9
mov    r9,0x0
adc    r9,rdx
mul    QWORD [const_38]
add    r9,rax
mov    rax,r10
mov    r10,0x0
adc    r10,rdx
mul    QWORD [const_38]
add    r10,rax
mov    rax,0x0
adc    rax,rdx
add    r12,rcx
adc    r13,r8
adc    r14,r9
adc    r15,r10
mov    rdx,0x0
adc    rax,rdx
imul   rcx,rax,0x26
add    r12,rcx
adc    r13,rdx
adc    r14,rdx
adc    r15,rdx
adc    rdx,rdx
imul   rdx,rdx,0x26
add    r12,rdx
mov    QWORD [rdi],r12
mov    QWORD [rdi+0x8],r13
mov    QWORD [rdi+0x10],r14
mov    QWORD [rdi+0x18],r15
mov    rcx,0x0
mov    r8,0x0
mov    r9,0x0
mov    r10,0x0
mov    r11,QWORD [rsi+0x40]
mov    rax,QWORD [rsi+0x20]
mul    r11
mov    r12,rax
mov    r13,rdx
mov    rax,QWORD [rsi+0x28]
mul    r11
add    r13,rax
mov    r14,0x0
adc    r14,rdx
mov    rax,QWORD [rsi+0x30]
mul    r11
add    r14,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,QWORD [rsi+0x38]
mul    r11
add    r15,rax
adc    rcx,rdx
mov    r11,QWORD [rsi+0x48]
mov    rax,QWORD [rsi+0x20]
mul    r11
add    r13,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x28]
mul    r11
add    r14,rax
adc    rdx,0x0
add    r14,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x30]
mul    r11
add    r15,rax
adc    rdx,0x0
add    r15,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x38]
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
adc    r8,rdx
mov    r11,QWORD [rsi+0x50]
mov    rax,QWORD [rsi+0x20]
mul    r11
add    r14,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x28]
mul    r11
add    r15,rax
adc    rdx,0x0
add    r15,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x30]
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x38]
mul    r11
add    r8,rax
adc    rdx,0x0
add    r8,rbx
adc    r9,rdx
mov    r11,QWORD [rsi+0x58]
mov    rax,QWORD [rsi+0x20]
mul    r11
add    r15,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x28]
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x30]
mul    r11
add    r8,rax
adc    rdx,0x0
add    r8,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x38]
mul    r11
add    r9,rax
adc    rdx,0x0
add    r9,rbx
adc    r10,rdx
mov    rax,rcx
mul    QWORD [const_38]
mov    rcx,rax
mov    rax,r8
mov    r8,rdx
mul    QWORD [const_38]
add    r8,rax
mov    rax,r9
mov    r9,0x0
adc    r9,rdx
mul    QWORD [const_38]
add    r9,rax
mov    rax,r10
mov    r10,0x0
adc    r10,rdx
mul    QWORD [const_38]
add    r10,rax
mov    rax,0x0
adc    rax,rdx
add    r12,rcx
adc    r13,r8
adc    r14,r9
adc    r15,r10
mov    rdx,0x0
adc    rax,rdx
imul   rcx,rax,0x26
add    r12,rcx
adc    r13,rdx
adc    r14,rdx
adc    r15,rdx
adc    rdx,rdx
imul   rdx,rdx,0x26
add    r12,rdx
mov    QWORD [rdi+0x20],r12
mov    QWORD [rdi+0x28],r13
mov    QWORD [rdi+0x30],r14
mov    QWORD [rdi+0x38],r15
mov    rcx,0x0
mov    r8,0x0
mov    r9,0x0
mov    r10,0x0
mov    r11,QWORD [rsi+0x20]
mov    rax,QWORD [rsi+0x60]
mul    r11
mov    r12,rax
mov    r13,rdx
mov    rax,QWORD [rsi+0x68]
mul    r11
add    r13,rax
mov    r14,0x0
adc    r14,rdx
mov    rax,QWORD [rsi+0x70]
mul    r11
add    r14,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,QWORD [rsi+0x78]
mul    r11
add    r15,rax
adc    rcx,rdx
mov    r11,QWORD [rsi+0x28]
mov    rax,QWORD [rsi+0x60]
mul    r11
add    r13,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x68]
mul    r11
add    r14,rax
adc    rdx,0x0
add    r14,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x70]
mul    r11
add    r15,rax
adc    rdx,0x0
add    r15,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x78]
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
adc    r8,rdx
mov    r11,QWORD [rsi+0x30]
mov    rax,QWORD [rsi+0x60]
mul    r11
add    r14,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x68]
mul    r11
add    r15,rax
adc    rdx,0x0
add    r15,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x70]
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x78]
mul    r11
add    r8,rax
adc    rdx,0x0
add    r8,rbx
adc    r9,rdx
mov    r11,QWORD [rsi+0x38]
mov    rax,QWORD [rsi+0x60]
mul    r11
add    r15,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x68]
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x70]
mul    r11
add    r8,rax
adc    rdx,0x0
add    r8,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x78]
mul    r11
add    r9,rax
adc    rdx,0x0
add    r9,rbx
adc    r10,rdx
mov    rax,rcx
mul    QWORD [const_38]
mov    rcx,rax
mov    rax,r8
mov    r8,rdx
mul    QWORD [const_38]
add    r8,rax
mov    rax,r9
mov    r9,0x0
adc    r9,rdx
mul    QWORD [const_38]
add    r9,rax
mov    rax,r10
mov    r10,0x0
adc    r10,rdx
mul    QWORD [const_38]
add    r10,rax
mov    rax,0x0
adc    rax,rdx
add    r12,rcx
adc    r13,r8
adc    r14,r9
adc    r15,r10
mov    rdx,0x0
adc    rax,rdx
imul   rcx,rax,0x26
add    r12,rcx
adc    r13,rdx
adc    r14,rdx
adc    r15,rdx
adc    rdx,rdx
imul   rdx,rdx,0x26
add    r12,rdx
mov    QWORD [rdi+0x40],r12
mov    QWORD [rdi+0x48],r13
mov    QWORD [rdi+0x50],r14
mov    QWORD [rdi+0x58],r15
mov    rcx,0x0
mov    r8,0x0
mov    r9,0x0
mov    r10,0x0
mov    r11,QWORD [rsi]
mov    rax,QWORD [rsi+0x40]
mul    r11
mov    r12,rax
mov    r13,rdx
mov    rax,QWORD [rsi+0x48]
mul    r11
add    r13,rax
mov    r14,0x0
adc    r14,rdx
mov    rax,QWORD [rsi+0x50]
mul    r11
add    r14,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,QWORD [rsi+0x58]
mul    r11
add    r15,rax
adc    rcx,rdx
mov    r11,QWORD [rsi+0x8]
mov    rax,QWORD [rsi+0x40]
mul    r11
add    r13,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x48]
mul    r11
add    r14,rax
adc    rdx,0x0
add    r14,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x50]
mul    r11
add    r15,rax
adc    rdx,0x0
add    r15,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x58]
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
adc    r8,rdx
mov    r11,QWORD [rsi+0x10]
mov    rax,QWORD [rsi+0x40]
mul    r11
add    r14,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x48]
mul    r11
add    r15,rax
adc    rdx,0x0
add    r15,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x50]
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x58]
mul    r11
add    r8,rax
adc    rdx,0x0
add    r8,rbx
adc    r9,rdx
mov    r11,QWORD [rsi+0x18]
mov    rax,QWORD [rsi+0x40]
mul    r11
add    r15,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x48]
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x50]
mul    r11
add    r8,rax
adc    rdx,0x0
add    r8,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x58]
mul    r11
add    r9,rax
adc    rdx,0x0
add    r9,rbx
adc    r10,rdx
mov    rax,rcx
mul    QWORD [const_38]
mov    rsi,rax
mov    rax,r8
mov    rcx,rdx
mul    QWORD [const_38]
add    rcx,rax
mov    rax,r9
mov    r8,0x0
adc    r8,rdx
mul    QWORD [const_38]
add    r8,rax
mov    rax,r10
mov    r9,0x0
adc    r9,rdx
mul    QWORD [const_38]
add    r9,rax
mov    rax,0x0
adc    rax,rdx
add    r12,rsi
adc    r13,rcx
adc    r14,r8
adc    r15,r9
mov    rsi,0x0
adc    rax,rsi
imul   rdx,rax,0x26
add    r12,rdx
adc    r13,rsi
adc    r14,rsi
adc    r15,rsi
adc    rsi,rsi
imul   rsi,rsi,0x26
add    r12,rsi
mov    QWORD [rdi+0x60],r12
mov    QWORD [rdi+0x68],r13
mov    QWORD [rdi+0x70],r14
mov    QWORD [rdi+0x78],r15
mov    r11,QWORD [rsp]
mov    r12,QWORD [rsp+0x8]
mov    r13,QWORD [rsp+0x10]
mov    r14,QWORD [rsp+0x18]
mov    r15,QWORD [rsp+0x20]
mov    rbx,QWORD [rsp+0x28]
mov    rbp,QWORD [rsp+0x30]
add    rsp,r11
mov    rax,rdi
mov    rdx,rsi
ret

sV_ge64_p1p1_to_p2:
mov    r11,rsp
and    r11,0x1f
add    r11,0x40
sub    rsp,r11
mov    QWORD [rsp],r11
mov    QWORD [rsp+0x8],r12
mov    QWORD [rsp+0x10],r13
mov    QWORD [rsp+0x18],r14
mov    QWORD [rsp+0x20],r15
mov    QWORD [rsp+0x28],rbx
mov    QWORD [rsp+0x30],rbp
mov    rcx,0x0
mov    r8,0x0
mov    r9,0x0
mov    r10,0x0
mov    r11,QWORD [rsi]
mov    rax,QWORD [rsi+0x60]
mul    r11
mov    r12,rax
mov    r13,rdx
mov    rax,QWORD [rsi+0x68]
mul    r11
add    r13,rax
mov    r14,0x0
adc    r14,rdx
mov    rax,QWORD [rsi+0x70]
mul    r11
add    r14,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,QWORD [rsi+0x78]
mul    r11
add    r15,rax
adc    rcx,rdx
mov    r11,QWORD [rsi+0x8]
mov    rax,QWORD [rsi+0x60]
mul    r11
add    r13,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x68]
mul    r11
add    r14,rax
adc    rdx,0x0
add    r14,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x70]
mul    r11
add    r15,rax
adc    rdx,0x0
add    r15,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x78]
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
adc    r8,rdx
mov    r11,QWORD [rsi+0x10]
mov    rax,QWORD [rsi+0x60]
mul    r11
add    r14,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x68]
mul    r11
add    r15,rax
adc    rdx,0x0
add    r15,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x70]
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x78]
mul    r11
add    r8,rax
adc    rdx,0x0
add    r8,rbx
adc    r9,rdx
mov    r11,QWORD [rsi+0x18]
mov    rax,QWORD [rsi+0x60]
mul    r11
add    r15,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x68]
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x70]
mul    r11
add    r8,rax
adc    rdx,0x0
add    r8,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x78]
mul    r11
add    r9,rax
adc    rdx,0x0
add    r9,rbx
adc    r10,rdx
mov    rax,rcx
mul    QWORD [const_38]
mov    rcx,rax
mov    rax,r8
mov    r8,rdx
mul    QWORD [const_38]
add    r8,rax
mov    rax,r9
mov    r9,0x0
adc    r9,rdx
mul    QWORD [const_38]
add    r9,rax
mov    rax,r10
mov    r10,0x0
adc    r10,rdx
mul    QWORD [const_38]
add    r10,rax
mov    rax,0x0
adc    rax,rdx
add    r12,rcx
adc    r13,r8
adc    r14,r9
adc    r15,r10
mov    rdx,0x0
adc    rax,rdx
imul   rcx,rax,0x26
add    r12,rcx
adc    r13,rdx
adc    r14,rdx
adc    r15,rdx
adc    rdx,rdx
imul   rdx,rdx,0x26
add    r12,rdx
mov    QWORD [rdi],r12
mov    QWORD [rdi+0x8],r13
mov    QWORD [rdi+0x10],r14
mov    QWORD [rdi+0x18],r15
mov    rcx,0x0
mov    r8,0x0
mov    r9,0x0
mov    r10,0x0
mov    r11,QWORD [rsi+0x40]
mov    rax,QWORD [rsi+0x20]
mul    r11
mov    r12,rax
mov    r13,rdx
mov    rax,QWORD [rsi+0x28]
mul    r11
add    r13,rax
mov    r14,0x0
adc    r14,rdx
mov    rax,QWORD [rsi+0x30]
mul    r11
add    r14,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,QWORD [rsi+0x38]
mul    r11
add    r15,rax
adc    rcx,rdx
mov    r11,QWORD [rsi+0x48]
mov    rax,QWORD [rsi+0x20]
mul    r11
add    r13,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x28]
mul    r11
add    r14,rax
adc    rdx,0x0
add    r14,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x30]
mul    r11
add    r15,rax
adc    rdx,0x0
add    r15,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x38]
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
adc    r8,rdx
mov    r11,QWORD [rsi+0x50]
mov    rax,QWORD [rsi+0x20]
mul    r11
add    r14,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x28]
mul    r11
add    r15,rax
adc    rdx,0x0
add    r15,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x30]
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x38]
mul    r11
add    r8,rax
adc    rdx,0x0
add    r8,rbx
adc    r9,rdx
mov    r11,QWORD [rsi+0x58]
mov    rax,QWORD [rsi+0x20]
mul    r11
add    r15,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x28]
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x30]
mul    r11
add    r8,rax
adc    rdx,0x0
add    r8,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x38]
mul    r11
add    r9,rax
adc    rdx,0x0
add    r9,rbx
adc    r10,rdx
mov    rax,rcx
mul    QWORD [const_38]
mov    rcx,rax
mov    rax,r8
mov    r8,rdx
mul    QWORD [const_38]
add    r8,rax
mov    rax,r9
mov    r9,0x0
adc    r9,rdx
mul    QWORD [const_38]
add    r9,rax
mov    rax,r10
mov    r10,0x0
adc    r10,rdx
mul    QWORD [const_38]
add    r10,rax
mov    rax,0x0
adc    rax,rdx
add    r12,rcx
adc    r13,r8
adc    r14,r9
adc    r15,r10
mov    rdx,0x0
adc    rax,rdx
imul   rcx,rax,0x26
add    r12,rcx
adc    r13,rdx
adc    r14,rdx
adc    r15,rdx
adc    rdx,rdx
imul   rdx,rdx,0x26
add    r12,rdx
mov    QWORD [rdi+0x20],r12
mov    QWORD [rdi+0x28],r13
mov    QWORD [rdi+0x30],r14
mov    QWORD [rdi+0x38],r15
mov    rcx,0x0
mov    r8,0x0
mov    r9,0x0
mov    r10,0x0
mov    r11,QWORD [rsi+0x20]
mov    rax,QWORD [rsi+0x60]
mul    r11
mov    r12,rax
mov    r13,rdx
mov    rax,QWORD [rsi+0x68]
mul    r11
add    r13,rax
mov    r14,0x0
adc    r14,rdx
mov    rax,QWORD [rsi+0x70]
mul    r11
add    r14,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,QWORD [rsi+0x78]
mul    r11
add    r15,rax
adc    rcx,rdx
mov    r11,QWORD [rsi+0x28]
mov    rax,QWORD [rsi+0x60]
mul    r11
add    r13,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x68]
mul    r11
add    r14,rax
adc    rdx,0x0
add    r14,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x70]
mul    r11
add    r15,rax
adc    rdx,0x0
add    r15,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x78]
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
adc    r8,rdx
mov    r11,QWORD [rsi+0x30]
mov    rax,QWORD [rsi+0x60]
mul    r11
add    r14,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x68]
mul    r11
add    r15,rax
adc    rdx,0x0
add    r15,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x70]
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x78]
mul    r11
add    r8,rax
adc    rdx,0x0
add    r8,rbx
adc    r9,rdx
mov    r11,QWORD [rsi+0x38]
mov    rax,QWORD [rsi+0x60]
mul    r11
add    r15,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x68]
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x70]
mul    r11
add    r8,rax
adc    rdx,0x0
add    r8,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x78]
mul    r11
add    r9,rax
adc    rdx,0x0
add    r9,rbx
adc    r10,rdx
mov    rax,rcx
mul    QWORD [const_38]
mov    rsi,rax
mov    rax,r8
mov    rcx,rdx
mul    QWORD [const_38]
add    rcx,rax
mov    rax,r9
mov    r8,0x0
adc    r8,rdx
mul    QWORD [const_38]
add    r8,rax
mov    rax,r10
mov    r9,0x0
adc    r9,rdx
mul    QWORD [const_38]
add    r9,rax
mov    rax,0x0
adc    rax,rdx
add    r12,rsi
adc    r13,rcx
adc    r14,r8
adc    r15,r9
mov    rsi,0x0
adc    rax,rsi
imul   rdx,rax,0x26
add    r12,rdx
adc    r13,rsi
adc    r14,rsi
adc    r15,rsi
adc    rsi,rsi
imul   rsi,rsi,0x26
add    r12,rsi
mov    QWORD [rdi+0x40],r12
mov    QWORD [rdi+0x48],r13
mov    QWORD [rdi+0x50],r14
mov    QWORD [rdi+0x58],r15
mov    r11,QWORD [rsp]
mov    r12,QWORD [rsp+0x8]
mov    r13,QWORD [rsp+0x10]
mov    r14,QWORD [rsp+0x18]
mov    r15,QWORD [rsp+0x20]
mov    rbx,QWORD [rsp+0x28]
mov    rbp,QWORD [rsp+0x30]
add    rsp,r11
mov    rax,rdi
mov    rdx,rsi
ret

sV_ge64_nielsadd2:
mov    r11,rsp
and    r11,0x1f
add    r11,0xc0
sub    rsp,r11
mov    QWORD [rsp],r11
mov    QWORD [rsp+0x8],r12
mov    QWORD [rsp+0x10],r13
mov    QWORD [rsp+0x18],r14
mov    QWORD [rsp+0x20],r15
mov    QWORD [rsp+0x28],rbx
mov    QWORD [rsp+0x30],rbp
mov    r11,QWORD [rdi+0x20]
mov    r8,QWORD [rdi+0x28]
mov    r9,QWORD [rdi+0x30]
mov    r10,QWORD [rdi+0x38]
mov    rax,r11
mov    rcx,r8
mov    rdx,r9
mov    r12,r10
xor    r13,r13
mov    r14,0x26
sub    r11,QWORD [rdi]
sbb    r8,QWORD [rdi+0x8]
sbb    r9,QWORD [rdi+0x10]
sbb    r10,QWORD [rdi+0x18]
cmovae r14,r13
sub    r11,r14
sbb    r8,r13
sbb    r9,r13
sbb    r10,r13
cmovb  r13,r14
sub    r11,r13
xor    r13,r13
mov    r14,0x26
add    rax,QWORD [rdi]
adc    rcx,QWORD [rdi+0x8]
adc    rdx,QWORD [rdi+0x10]
adc    r12,QWORD [rdi+0x18]
cmovae r14,r13
add    rax,r14
adc    rcx,r13
adc    rdx,r13
adc    r12,r13
cmovb  r13,r14
add    rax,r13
mov    QWORD [rsp+0x58],rax
mov    QWORD [rsp+0x60],rcx
mov    QWORD [rsp+0x68],rdx
mov    QWORD [rsp+0x70],r12
xor    rcx,rcx
mov    rax,QWORD [rsi]
mul    r11
mov    r12,rax
mov    r13,rdx
mov    rax,QWORD [rsi+0x8]
mul    r11
xor    r14,r14
add    r13,rax
adc    r14,rdx
mov    rax,QWORD [rsi+0x10]
mul    r11
xor    r15,r15
add    r14,rax
adc    r15,rdx
mov    rax,QWORD [rsi+0x18]
mul    r11
add    r15,rax
adc    rcx,rdx
mov    rax,QWORD [rsi]
mul    r8
xor    rbx,rbx
xor    r11,r11
add    r13,rax
adc    rbx,rdx
mov    rax,QWORD [rsi+0x8]
mul    r8
add    r14,rax
adc    rdx,r11
add    r14,rbx
adc    r11,rdx
mov    rax,QWORD [rsi+0x10]
mul    r8
xor    rbx,rbx
add    r15,rax
adc    rdx,rbx
add    r15,r11
adc    rbx,rdx
mov    rax,QWORD [rsi+0x18]
mul    r8
xor    r11,r11
add    rcx,rax
adc    rdx,r11
add    rcx,rbx
adc    r11,rdx
mov    rax,QWORD [rsi]
mul    r9
xor    r8,r8
xor    rbx,rbx
add    r14,rax
adc    rbx,rdx
mov    rax,QWORD [rsi+0x8]
mul    r9
add    r15,rax
adc    rdx,r8
add    r15,rbx
adc    r8,rdx
mov    rax,QWORD [rsi+0x10]
mul    r9
xor    rbx,rbx
add    rcx,rax
adc    rdx,rbx
add    rcx,r8
adc    rbx,rdx
mov    rax,QWORD [rsi+0x18]
mul    r9
xor    r9,r9
add    r11,rax
adc    rdx,r9
add    r11,rbx
adc    r9,rdx
mov    rax,QWORD [rsi]
mul    r10
xor    r8,r8
xor    rbx,rbx
add    r15,rax
adc    rbx,rdx
mov    rax,QWORD [rsi+0x8]
mul    r10
add    rcx,rax
adc    rdx,r8
add    rcx,rbx
adc    r8,rdx
mov    rax,QWORD [rsi+0x10]
mul    r10
xor    rbx,rbx
add    r11,rax
adc    rdx,rbx
add    r11,r8
adc    rbx,rdx
mov    rax,QWORD [rsi+0x18]
mul    r10
xor    r10,r10
add    r9,rax
adc    rdx,r10
add    r9,rbx
adc    r10,rdx
mov    rax,rcx
mov    rbp,0x26
mul    rbp
mov    rcx,rax
mov    rax,r11
mov    r8,rdx
mul    rbp
xor    rbx,rbx
add    r8,rax
mov    rax,r9
adc    rbx,rdx
mul    rbp
xor    r9,r9
add    rbx,rax
mov    rax,r10
adc    r9,rdx
mul    rbp
xor    r10,r10
add    r9,rax
adc    r10,rdx
xor    rax,rax
add    r12,rcx
adc    r13,r8
adc    r14,rbx
adc    r15,r9
adc    r10,rax
imul   rcx,r10,0x26
add    r12,rcx
adc    r13,rax
adc    r14,rax
adc    r15,rax
adc    rax,rax
imul   rax,rax,0x26
add    r12,rax
mov    QWORD [rsp+0x38],r12
mov    QWORD [rsp+0x40],r13
mov    QWORD [rsp+0x48],r14
mov    QWORD [rsp+0x50],r15
xor    rcx,rcx
mov    r10,QWORD [rsi+0x20]
mov    r9,QWORD [rsi+0x28]
mov    r8,QWORD [rsi+0x30]
mov    r11,QWORD [rsp+0x58]
mov    rax,r10
mul    r11
mov    r12,rax
mov    r13,rdx
mov    rax,r9
mul    r11
xor    r14,r14
add    r13,rax
adc    r14,rdx
mov    rax,r8
mul    r11
xor    r15,r15
add    r14,rax
adc    r15,rdx
mov    rax,QWORD [rsi+0x38]
mul    r11
add    r15,rax
adc    rcx,rdx
mov    r11,QWORD [rsp+0x60]
mov    rax,r10
mul    r11
xor    rbx,rbx
add    r13,rax
adc    rbx,rdx
mov    rax,r9
mul    r11
add    r14,rax
adc    rdx,0x0
add    r14,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,r8
mul    r11
add    r15,rax
adc    rdx,0x0
add    r15,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x38]
xor    r8,r8
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
adc    r8,rdx
mov    r11,QWORD [rsp+0x68]
mov    rax,r10
mul    r11
xor    rbx,rbx
add    r14,rax
adc    rbx,rdx
mov    rax,r9
mul    r11
add    r15,rax
adc    rdx,0x0
add    r15,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x30]
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x38]
xor    r9,r9
mul    r11
add    r8,rax
adc    rdx,0x0
add    r8,rbx
adc    r9,rdx
mov    r11,QWORD [rsp+0x70]
mov    rax,r10
mul    r11
xor    rbx,rbx
add    r15,rax
adc    rbx,rdx
mov    rax,QWORD [rsi+0x28]
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x30]
mul    r11
add    r8,rax
adc    rdx,0x0
add    r8,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x38]
mul    r11
xor    r10,r10
add    r9,rax
adc    rdx,0x0
add    r9,rbx
adc    r10,rdx
mov    rax,rcx
mov    rbp,0x26
mul    rbp
mov    rcx,rax
mov    rax,r8
mov    r8,rdx
mul    rbp
xor    rbx,rbx
add    r8,rax
mov    rax,r9
adc    rbx,rdx
mul    rbp
xor    r9,r9
add    rbx,rax
mov    rax,r10
adc    r9,rdx
mul    rbp
xor    r10,r10
add    r9,rax
adc    r10,rdx
xor    rax,rax
add    r12,rcx
adc    r13,r8
adc    r14,rbx
adc    r15,r9
adc    r10,rax
imul   rcx,r10,0x26
add    r12,rcx
adc    r13,rax
adc    r14,rax
adc    r15,rax
adc    rax,rax
imul   rax,rax,0x26
add    r12,rax
mov    rdx,r12
mov    rcx,r13
mov    r8,r14
mov    r9,r15
mov    r10,QWORD [rsp+0x38]
mov    r11,QWORD [rsp+0x40]
mov    rbx,QWORD [rsp+0x48]
xor    rax,rax
sub    r12,r10
sbb    r13,r11
sbb    r14,rbx
sbb    r15,QWORD [rsp+0x50]
cmovae rbp,rax
sub    r12,rbp
sbb    r13,rax
sbb    r14,rax
sbb    r15,rax
cmovb  rax,rbp
sub    r12,rax
xor    rax,rax
add    rdx,r10
adc    rcx,r11
adc    r8,rbx
adc    r9,QWORD [rsp+0x50]
mov    r10,0x26
cmovae r10,rax
add    rdx,r10
adc    rcx,rax
adc    r8,rax
adc    r9,rax
cmovb  rax,r10
add    rdx,rax
mov    QWORD [rsp+0x38],rdx
mov    QWORD [rsp+0x40],rcx
mov    QWORD [rsp+0x48],r8
mov    QWORD [rsp+0x50],r9
mov    QWORD [rsp+0x58],r12
mov    QWORD [rsp+0x60],r13
mov    QWORD [rsp+0x68],r14
mov    QWORD [rsp+0x70],r15
xor    rcx,rcx
xor    r8,r8
xor    r9,r9
mov    r10,QWORD [rsi+0x40]
mov    rbp,QWORD [rsi+0x48]
mov    r11,QWORD [rdi+0x60]
mov    rax,r10
mul    r11
mov    r12,rax
mov    r13,rdx
mov    rax,rbp
mul    r11
xor    r14,r14
add    r13,rax
adc    r14,rdx
mov    rax,QWORD [rsi+0x50]
mul    r11
xor    r15,r15
add    r14,rax
adc    r15,rdx
mov    rax,QWORD [rsi+0x58]
mul    r11
add    r15,rax
adc    rcx,rdx
mov    r11,QWORD [rdi+0x68]
mov    rax,r10
mul    r11
xor    rbx,rbx
add    r13,rax
adc    rbx,rdx
mov    rax,rbp
mul    r11
add    r14,rax
adc    rdx,0x0
add    r14,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x50]
mul    r11
add    r15,rax
adc    rdx,0x0
add    r15,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x58]
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
adc    r8,rdx
mov    r11,QWORD [rdi+0x70]
mov    rax,r10
mul    r11
add    r14,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,rbp
mul    r11
add    r15,rax
adc    rdx,0x0
add    r15,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x50]
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x58]
mul    r11
add    r8,rax
adc    rdx,0x0
add    r8,rbx
adc    r9,rdx
mov    r11,QWORD [rdi+0x78]
mov    rax,r10
mul    r11
add    r15,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,rbp
mul    r11
add    rcx,rax
adc    rdx,0x0
add    rcx,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x50]
mul    r11
add    r8,rax
adc    rdx,0x0
add    r8,rbx
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rsi+0x58]
mul    r11
xor    r10,r10
add    r9,rax
adc    rdx,0x0
add    r9,rbx
adc    r10,rdx
mov    rax,rcx
mov    rbp,0x26
mul    rbp
mov    rsi,rax
mov    rax,r8
mov    rcx,rdx
mul    rbp
xor    rbx,rbx
add    rcx,rax
mov    rax,r9
adc    rbx,rdx
mul    rbp
xor    r8,r8
add    rbx,rax
mov    rax,r10
adc    r8,rdx
mul    rbp
xor    r9,r9
add    r8,rax
adc    r9,rdx
xor    rax,rax
add    r12,rsi
adc    r13,rcx
adc    r14,rbx
adc    r15,r8
adc    r9,rax
imul   rdx,r9,0x26
add    r12,rdx
adc    r13,rax
adc    r14,rax
adc    r15,rax
adc    rax,rax
imul   rax,rax,0x26
add    r12,rax
mov    r9,QWORD [rdi+0x40]
mov    rbx,QWORD [rdi+0x48]
mov    rcx,QWORD [rdi+0x50]
mov    r8,QWORD [rdi+0x58]
xor    rsi,rsi
add    r9,r9
adc    rbx,rbx
adc    rcx,rcx
adc    r8,r8
mov    rax,rbp
cmovae rax,rsi
add    r9,rax
adc    rbx,rsi
adc    rcx,rsi
adc    r8,rsi
cmovb  rsi,rax
add    r9,rsi
mov    rsi,r9
mov    rax,rbx
mov    r10,rcx
mov    r11,r8
xor    rdx,rdx
sub    r9,r12
sbb    rbx,r13
sbb    rcx,r14
sbb    r8,r15
cmovae rbp,rdx
sub    r9,rbp
sbb    rbx,rdx
sbb    rcx,rdx
sbb    r8,rdx
cmovb  rdx,rbp
sub    r9,rdx
add    rsi,r12
adc    rax,r13
adc    r10,r14
adc    r11,r15
mov    r12,0x0
mov    r13,0x26
cmovae r13,r12
add    rsi,r13
adc    rax,r12
adc    r10,r12
adc    r11,r12
cmovb  r12,r13
add    rsi,r12
mov    QWORD [rsp+0x78],rsi
mov    QWORD [rsp+0x80],rax
mov    QWORD [rsp+0x88],r10
mov    QWORD [rsp+0x90],r11
mov    QWORD [rsp+0x98],r9
mov    QWORD [rsp+0xa0],rbx
mov    QWORD [rsp+0xa8],rcx
mov    QWORD [rsp+0xb0],r8
mov    rbp,rcx
xor    rsi,rsi
xor    rcx,rcx
mov    r10,QWORD [rsp+0x58]
mov    rax,r9
mul    r10
mov    r11,rax
mov    r12,rdx
mov    rax,rbx
mul    r10
add    r12,rax
mov    r13,0x0
adc    r13,rdx
mov    rax,rbp
mul    r10
add    r13,rax
mov    r14,0x0
adc    r14,rdx
mov    rax,r8
mul    r10
add    r14,rax
adc    rsi,rdx
mov    r10,QWORD [rsp+0x60]
mov    rax,r9
mul    r10
xor    r15,r15
add    r12,rax
adc    r15,rdx
mov    rax,rbx
mul    r10
add    r13,rax
adc    rdx,0x0
add    r13,r15
mov    r15,0x0
adc    r15,rdx
mov    rax,rbp
mul    r10
add    r14,rax
adc    rdx,0x0
add    r14,r15
mov    r15,0x0
adc    r15,rdx
mov    rax,r8
mul    r10
add    rsi,rax
adc    rdx,0x0
add    rsi,r15
adc    rcx,rdx
mov    r10,QWORD [rsp+0x68]
mov    rax,r9
mul    r10
add    r13,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,rbx
mul    r10
add    r14,rax
adc    rdx,0x0
add    r14,r15
mov    r15,0x0
adc    r15,rdx
mov    rax,rbp
mul    r10
add    rsi,rax
adc    rdx,0x0
add    rsi,r15
mov    r15,0x0
adc    r15,rdx
mov    rax,r8
mul    r10
xor    r8,r8
add    rcx,rax
adc    rdx,0x0
add    rcx,r15
adc    r8,rdx
mov    r10,QWORD [rsp+0x70]
mov    rax,r9
mul    r10
add    r14,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,rbx
mul    r10
add    rsi,rax
adc    rdx,0x0
add    rsi,r15
mov    r15,0x0
adc    r15,rdx
mov    rax,rbp
mul    r10
add    rcx,rax
adc    rdx,0x0
add    rcx,r15
mov    r15,0x0
adc    r15,rdx
mov    rax,QWORD [rsp+0xb0]
mul    r10
xor    r9,r9
add    r8,rax
adc    rdx,0x0
add    r8,r15
adc    r9,rdx
mov    rax,rsi
mov    rbp,0x26
mul    rbp
mov    rsi,rax
mov    rax,rcx
mov    rcx,rdx
mul    rbp
xor    rbx,rbx
add    rcx,rax
mov    rax,r8
adc    rbx,rdx
mul    rbp
xor    r8,r8
add    rbx,rax
mov    rax,r9
adc    r8,rdx
mul    rbp
xor    r9,r9
add    r8,rax
adc    r9,rdx
xor    rax,rax
add    r11,rsi
adc    r12,rcx
adc    r13,rbx
adc    r14,r8
adc    r9,rax
imul   rdx,r9,0x26
add    r11,rdx
adc    r12,rax
adc    r13,rax
adc    r14,rax
adc    rax,rax
imul   rax,rax,0x26
add    r11,rax
mov    QWORD [rdi],r11
mov    QWORD [rdi+0x8],r12
mov    QWORD [rdi+0x10],r13
mov    QWORD [rdi+0x18],r14
xor    rsi,rsi
xor    rcx,rcx
mov    r9,QWORD [rsp+0x78]
mov    rbx,QWORD [rsp+0x80]
mov    rbp,QWORD [rsp+0x88]
mov    r8,QWORD [rsp+0x90]
mov    r10,QWORD [rsp+0x38]
mov    rax,r9
mul    r10
mov    r11,rax
mov    r12,rdx
mov    rax,rbx
mul    r10
add    r12,rax
mov    r13,0x0
adc    r13,rdx
mov    rax,rbp
mul    r10
add    r13,rax
mov    r14,0x0
adc    r14,rdx
mov    rax,r8
mul    r10
add    r14,rax
adc    rsi,rdx
mov    r10,QWORD [rsp+0x40]
mov    rax,r9
mul    r10
add    r12,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,rbx
mul    r10
add    r13,rax
adc    rdx,0x0
add    r13,r15
mov    r15,0x0
adc    r15,rdx
mov    rax,rbp
mul    r10
add    r14,rax
adc    rdx,0x0
add    r14,r15
mov    r15,0x0
adc    r15,rdx
mov    rax,r8
mul    r10
add    rsi,rax
adc    rdx,0x0
add    rsi,r15
adc    rcx,rdx
mov    r10,QWORD [rsp+0x48]
mov    rax,r9
mul    r10
add    r13,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,rbx
mul    r10
add    r14,rax
adc    rdx,0x0
add    r14,r15
mov    r15,0x0
adc    r15,rdx
mov    rax,rbp
mul    r10
add    rsi,rax
adc    rdx,0x0
add    rsi,r15
mov    r15,0x0
adc    r15,rdx
mov    rax,r8
mul    r10
xor    r8,r8
add    rcx,rax
adc    rdx,0x0
add    rcx,r15
adc    r8,rdx
mov    r10,QWORD [rsp+0x50]
mov    rax,r9
mul    r10
add    r14,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,rbx
mul    r10
add    rsi,rax
adc    rdx,0x0
add    rsi,r15
mov    r15,0x0
adc    r15,rdx
mov    rax,rbp
mul    r10
add    rcx,rax
adc    rdx,0x0
add    rcx,r15
mov    r15,0x0
adc    r15,rdx
mov    rax,QWORD [rsp+0x90]
mul    r10
xor    r9,r9
add    r8,rax
adc    rdx,0x0
add    r8,r15
adc    r9,rdx
mov    rax,rsi
mov    rbp,0x26
mul    rbp
mov    rsi,rax
mov    rax,rcx
mov    rcx,rdx
mul    rbp
xor    rbx,rbx
add    rcx,rax
mov    rax,r8
adc    rbx,rdx
mul    rbp
xor    r8,r8
add    rbx,rax
mov    rax,r9
adc    r8,rdx
mul    rbp
xor    r9,r9
add    r8,rax
adc    r9,rdx
xor    rax,rax
add    r11,rsi
adc    r12,rcx
adc    r13,rbx
adc    r14,r8
adc    r9,rax
imul   rdx,r9,0x26
add    r11,rdx
adc    r12,rax
adc    r13,rax
adc    r14,rax
adc    rax,rax
imul   rax,rax,0x26
add    r11,rax
mov    QWORD [rdi+0x20],r11
mov    QWORD [rdi+0x28],r12
mov    QWORD [rdi+0x30],r13
mov    QWORD [rdi+0x38],r14
xor    rsi,rsi
xor    rcx,rcx
mov    r9,QWORD [rsp+0x98]
mov    rbx,QWORD [rsp+0xa0]
mov    rbp,QWORD [rsp+0xa8]
mov    r8,QWORD [rsp+0xb0]
mov    r10,QWORD [rsp+0x78]
mov    rax,r9
mul    r10
mov    r11,rax
mov    r12,rdx
mov    rax,rbx
mul    r10
add    r12,rax
mov    r13,0x0
adc    r13,rdx
mov    rax,rbp
mul    r10
add    r13,rax
mov    r14,0x0
adc    r14,rdx
mov    rax,r8
mul    r10
add    r14,rax
adc    rsi,rdx
mov    r10,QWORD [rsp+0x80]
mov    rax,r9
mul    r10
add    r12,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,rbx
mul    r10
add    r13,rax
adc    rdx,0x0
add    r13,r15
mov    r15,0x0
adc    r15,rdx
mov    rax,rbp
mul    r10
add    r14,rax
adc    rdx,0x0
add    r14,r15
mov    r15,0x0
adc    r15,rdx
mov    rax,r8
mul    r10
add    rsi,rax
adc    rdx,0x0
add    rsi,r15
adc    rcx,rdx
mov    r10,QWORD [rsp+0x88]
mov    rax,r9
mul    r10
add    r13,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,rbx
mul    r10
add    r14,rax
adc    rdx,0x0
add    r14,r15
mov    r15,0x0
adc    r15,rdx
mov    rax,rbp
mul    r10
add    rsi,rax
adc    rdx,0x0
add    rsi,r15
mov    r15,0x0
adc    r15,rdx
mov    rax,r8
mul    r10
xor    r8,r8
add    rcx,rax
adc    rdx,0x0
add    rcx,r15
adc    r8,rdx
mov    r10,QWORD [rsp+0x90]
mov    rax,r9
mul    r10
add    r14,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,rbx
mul    r10
add    rsi,rax
adc    rdx,0x0
add    rsi,r15
mov    r15,0x0
adc    r15,rdx
mov    rax,rbp
mul    r10
add    rcx,rax
adc    rdx,0x0
add    rcx,r15
mov    r15,0x0
adc    r15,rdx
mov    rax,QWORD [rsp+0xb0]
mul    r10
xor    r9,r9
add    r8,rax
adc    rdx,0x0
add    r8,r15
adc    r9,rdx
mov    rax,rsi
mov    rbp,0x26
mul    rbp
mov    rsi,rax
mov    rax,rcx
mov    rcx,rdx
mul    rbp
xor    rbx,rbx
add    rcx,rax
mov    rax,r8
adc    rbx,rdx
mul    rbp
xor    r8,r8
add    rbx,rax
mov    rax,r9
adc    r8,rdx
mul    rbp
xor    r9,r9
add    r8,rax
adc    r9,rdx
xor    rax,rax
add    r11,rsi
adc    r12,rcx
adc    r13,rbx
adc    r14,r8
adc    r9,rax
imul   rdx,r9,0x26
add    r11,rdx
adc    r12,rax
adc    r13,rax
adc    r14,rax
adc    rax,rax
imul   rax,rax,0x26
add    r11,rax
mov    QWORD [rdi+0x40],r11
mov    QWORD [rdi+0x48],r12
mov    QWORD [rdi+0x50],r13
mov    QWORD [rdi+0x58],r14
xor    rsi,rsi
xor    rcx,rcx
mov    r9,QWORD [rsp+0x38]
mov    rbx,QWORD [rsp+0x40]
mov    rbp,QWORD [rsp+0x48]
mov    r8,QWORD [rsp+0x50]
mov    r10,QWORD [rsp+0x58]
mov    rax,r9
mul    r10
mov    r11,rax
mov    r12,rdx
mov    rax,rbx
mul    r10
xor    r13,r13
add    r12,rax
adc    r13,rdx
mov    rax,rbp
mul    r10
xor    r14,r14
add    r13,rax
adc    r14,rdx
mov    rax,r8
mul    r10
add    r14,rax
adc    rsi,rdx
mov    r10,QWORD [rsp+0x60]
mov    rax,r9
mul    r10
xor    r15,r15
add    r12,rax
adc    r15,rdx
mov    rax,rbx
mul    r10
add    r13,rax
adc    rdx,0x0
add    r13,r15
mov    r15,0x0
adc    r15,rdx
mov    rax,rbp
mul    r10
add    r14,rax
adc    rdx,0x0
add    r14,r15
mov    r15,0x0
adc    r15,rdx
mov    rax,r8
mul    r10
add    rsi,rax
adc    rdx,0x0
add    rsi,r15
adc    rcx,rdx
mov    r10,QWORD [rsp+0x68]
mov    rax,r9
mul    r10
add    r13,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,rbx
mul    r10
add    r14,rax
adc    rdx,0x0
add    r14,r15
mov    r15,0x0
adc    r15,rdx
mov    rax,rbp
mul    r10
add    rsi,rax
adc    rdx,0x0
add    rsi,r15
mov    r15,0x0
adc    r15,rdx
mov    rax,r8
mul    r10
xor    r8,r8
add    rcx,rax
adc    rdx,0x0
add    rcx,r15
adc    r8,rdx
mov    r10,QWORD [rsp+0x70]
mov    rax,r9
mul    r10
add    r14,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,rbx
mul    r10
add    rsi,rax
adc    rdx,0x0
add    rsi,r15
mov    r15,0x0
adc    r15,rdx
mov    rax,rbp
mul    r10
add    rcx,rax
adc    rdx,0x0
add    rcx,r15
mov    r15,0x0
adc    r15,rdx
mov    rax,QWORD [rsp+0x50]
mul    r10
xor    r9,r9
add    r8,rax
adc    rdx,0x0
add    r8,r15
adc    r9,rdx
mov    rax,rsi
mov    rbp,0x26
mul    rbp
mov    rsi,rax
mov    rax,rcx
mov    rcx,rdx
mul    rbp
xor    rbx,rbx
add    rcx,rax
mov    rax,r8
adc    rbx,rdx
mul    rbp
xor    r8,r8
add    rbx,rax
mov    rax,r9
adc    r8,rdx
mul    rbp
xor    r9,r9
add    r8,rax
adc    r9,rdx
xor    rax,rax
add    r11,rsi
adc    r12,rcx
adc    r13,rbx
adc    r14,r8
adc    r9,rax
imul   rdx,r9,0x26
add    r11,rdx
adc    r12,rax
adc    r13,rax
adc    r14,rax
adc    rax,rax
imul   rax,rax,0x26
add    r11,rax
mov    QWORD [rdi+0x60],r11
mov    QWORD [rdi+0x68],r12
mov    QWORD [rdi+0x70],r13
mov    QWORD [rdi+0x78],r14
mov    r11,QWORD [rsp]
mov    r12,QWORD [rsp+0x8]
mov    r13,QWORD [rsp+0x10]
mov    r14,QWORD [rsp+0x18]
mov    r15,QWORD [rsp+0x20]
mov    rbx,QWORD [rsp+0x28]
mov    rbp,QWORD [rsp+0x30]
add    rsp,r11
mov    rax,rdi
mov    rdx,rsi
ret

sV_ge64_pnielsadd_p1p1:
mov    r11,rsp
and    r11,0x1f
add    r11,0x80
sub    rsp,r11
mov    QWORD [rsp],r11
mov    QWORD [rsp+0x8],r12
mov    QWORD [rsp+0x10],r13
mov    QWORD [rsp+0x18],r14
mov    QWORD [rsp+0x20],r15
mov    QWORD [rsp+0x28],rbx
mov    QWORD [rsp+0x30],rbp
mov    rcx,rdx
mov    rdx,QWORD [rsi+0x20]
mov    r8,QWORD [rsi+0x28]
mov    r9,QWORD [rsi+0x30]
mov    rax,QWORD [rsi+0x38]
mov    r10,rdx
mov    r11,r8
mov    r12,r9
mov    r13,rax
sub    rdx,QWORD [rsi]
sbb    r8,QWORD [rsi+0x8]
sbb    r9,QWORD [rsi+0x10]
sbb    rax,QWORD [rsi+0x18]
mov    r14,0x0
mov    r15,0x26
cmovae r15,r14
sub    rdx,r15
sbb    r8,r14
sbb    r9,r14
sbb    rax,r14
cmovb  r14,r15
sub    rdx,r14
add    r10,QWORD [rsi]
adc    r11,QWORD [rsi+0x8]
adc    r12,QWORD [rsi+0x10]
adc    r13,QWORD [rsi+0x18]
mov    r14,0x0
mov    r15,0x26
cmovae r15,r14
add    r10,r15
adc    r11,r14
adc    r12,r14
adc    r13,r14
cmovb  r14,r15
add    r10,r14
mov    QWORD [rsp+0x38],rdx
mov    QWORD [rsp+0x40],r8
mov    QWORD [rsp+0x48],r9
mov    QWORD [rsp+0x50],rax
mov    QWORD [rsp+0x58],r10
mov    QWORD [rsp+0x60],r11
mov    QWORD [rsp+0x68],r12
mov    QWORD [rsp+0x70],r13
mov    r8,0x0
mov    r9,0x0
mov    r10,0x0
mov    r11,0x0
mov    r12,QWORD [rsp+0x38]
mov    rax,QWORD [rcx]
mul    r12
mov    r13,rax
mov    r14,rdx
mov    rax,QWORD [rcx+0x8]
mul    r12
add    r14,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,QWORD [rcx+0x10]
mul    r12
add    r15,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rcx+0x18]
mul    r12
add    rbx,rax
adc    r8,rdx
mov    r12,QWORD [rsp+0x40]
mov    rax,QWORD [rcx]
mul    r12
add    r14,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x8]
mul    r12
add    r15,rax
adc    rdx,0x0
add    r15,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x10]
mul    r12
add    rbx,rax
adc    rdx,0x0
add    rbx,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x18]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
adc    r9,rdx
mov    r12,QWORD [rsp+0x48]
mov    rax,QWORD [rcx]
mul    r12
add    r15,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x8]
mul    r12
add    rbx,rax
adc    rdx,0x0
add    rbx,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x10]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x18]
mul    r12
add    r9,rax
adc    rdx,0x0
add    r9,rbp
adc    r10,rdx
mov    r12,QWORD [rsp+0x50]
mov    rax,QWORD [rcx]
mul    r12
add    rbx,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x8]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x10]
mul    r12
add    r9,rax
adc    rdx,0x0
add    r9,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x18]
mul    r12
add    r10,rax
adc    rdx,0x0
add    r10,rbp
adc    r11,rdx
mov    rax,r8
mul    QWORD [const_38]
mov    r8,rax
mov    rax,r9
mov    r9,rdx
mul    QWORD [const_38]
add    r9,rax
mov    rax,r10
mov    r10,0x0
adc    r10,rdx
mul    QWORD [const_38]
add    r10,rax
mov    rax,r11
mov    r11,0x0
adc    r11,rdx
mul    QWORD [const_38]
add    r11,rax
mov    rax,0x0
adc    rax,rdx
add    r13,r8
adc    r14,r9
adc    r15,r10
adc    rbx,r11
mov    rdx,0x0
adc    rax,rdx
imul   r8,rax,0x26
add    r13,r8
adc    r14,rdx
adc    r15,rdx
adc    rbx,rdx
adc    rdx,rdx
imul   rdx,rdx,0x26
add    r13,rdx
mov    QWORD [rsp+0x38],r13
mov    QWORD [rsp+0x40],r14
mov    QWORD [rsp+0x48],r15
mov    QWORD [rsp+0x50],rbx
mov    r8,0x0
mov    r9,0x0
mov    r10,0x0
mov    r11,0x0
mov    r12,QWORD [rsp+0x58]
mov    rax,QWORD [rcx+0x20]
mul    r12
mov    r13,rax
mov    r14,rdx
mov    rax,QWORD [rcx+0x28]
mul    r12
add    r14,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,QWORD [rcx+0x30]
mul    r12
add    r15,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rcx+0x38]
mul    r12
add    rbx,rax
adc    r8,rdx
mov    r12,QWORD [rsp+0x60]
mov    rax,QWORD [rcx+0x20]
mul    r12
add    r14,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x28]
mul    r12
add    r15,rax
adc    rdx,0x0
add    r15,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x30]
mul    r12
add    rbx,rax
adc    rdx,0x0
add    rbx,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x38]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
adc    r9,rdx
mov    r12,QWORD [rsp+0x68]
mov    rax,QWORD [rcx+0x20]
mul    r12
add    r15,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x28]
mul    r12
add    rbx,rax
adc    rdx,0x0
add    rbx,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x30]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x38]
mul    r12
add    r9,rax
adc    rdx,0x0
add    r9,rbp
adc    r10,rdx
mov    r12,QWORD [rsp+0x70]
mov    rax,QWORD [rcx+0x20]
mul    r12
add    rbx,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x28]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x30]
mul    r12
add    r9,rax
adc    rdx,0x0
add    r9,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x38]
mul    r12
add    r10,rax
adc    rdx,0x0
add    r10,rbp
adc    r11,rdx
mov    rax,r8
mul    QWORD [const_38]
mov    r8,rax
mov    rax,r9
mov    r9,rdx
mul    QWORD [const_38]
add    r9,rax
mov    rax,r10
mov    r10,0x0
adc    r10,rdx
mul    QWORD [const_38]
add    r10,rax
mov    rax,r11
mov    r11,0x0
adc    r11,rdx
mul    QWORD [const_38]
add    r11,rax
mov    rax,0x0
adc    rax,rdx
add    r13,r8
adc    r14,r9
adc    r15,r10
adc    rbx,r11
mov    rdx,0x0
adc    rax,rdx
imul   r8,rax,0x26
add    r13,r8
adc    r14,rdx
adc    r15,rdx
adc    rbx,rdx
adc    rdx,rdx
imul   rdx,rdx,0x26
add    r13,rdx
mov    rdx,r13
mov    r8,r14
mov    r9,r15
mov    rax,rbx
add    rdx,QWORD [rsp+0x38]
adc    r8,QWORD [rsp+0x40]
adc    r9,QWORD [rsp+0x48]
adc    rax,QWORD [rsp+0x50]
mov    r10,0x0
mov    r11,0x26
cmovae r11,r10
add    rdx,r11
adc    r8,r10
adc    r9,r10
adc    rax,r10
cmovb  r10,r11
add    rdx,r10
sub    r13,QWORD [rsp+0x38]
sbb    r14,QWORD [rsp+0x40]
sbb    r15,QWORD [rsp+0x48]
sbb    rbx,QWORD [rsp+0x50]
mov    r10,0x0
mov    r11,0x26
cmovae r11,r10
sub    r13,r11
sbb    r14,r10
sbb    r15,r10
sbb    rbx,r10
cmovb  r10,r11
sub    r13,r10
mov    QWORD [rdi],r13
mov    QWORD [rdi+0x8],r14
mov    QWORD [rdi+0x10],r15
mov    QWORD [rdi+0x18],rbx
mov    QWORD [rdi+0x40],rdx
mov    QWORD [rdi+0x48],r8
mov    QWORD [rdi+0x50],r9
mov    QWORD [rdi+0x58],rax
mov    r8,0x0
mov    r9,0x0
mov    r10,0x0
mov    r11,0x0
mov    r12,QWORD [rsi+0x60]
mov    rax,QWORD [rcx+0x60]
mul    r12
mov    r13,rax
mov    r14,rdx
mov    rax,QWORD [rcx+0x68]
mul    r12
add    r14,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,QWORD [rcx+0x70]
mul    r12
add    r15,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rcx+0x78]
mul    r12
add    rbx,rax
adc    r8,rdx
mov    r12,QWORD [rsi+0x68]
mov    rax,QWORD [rcx+0x60]
mul    r12
add    r14,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x68]
mul    r12
add    r15,rax
adc    rdx,0x0
add    r15,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x70]
mul    r12
add    rbx,rax
adc    rdx,0x0
add    rbx,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x78]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
adc    r9,rdx
mov    r12,QWORD [rsi+0x70]
mov    rax,QWORD [rcx+0x60]
mul    r12
add    r15,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x68]
mul    r12
add    rbx,rax
adc    rdx,0x0
add    rbx,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x70]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x78]
mul    r12
add    r9,rax
adc    rdx,0x0
add    r9,rbp
adc    r10,rdx
mov    r12,QWORD [rsi+0x78]
mov    rax,QWORD [rcx+0x60]
mul    r12
add    rbx,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x68]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x70]
mul    r12
add    r9,rax
adc    rdx,0x0
add    r9,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x78]
mul    r12
add    r10,rax
adc    rdx,0x0
add    r10,rbp
adc    r11,rdx
mov    rax,r8
mul    QWORD [const_38]
mov    r8,rax
mov    rax,r9
mov    r9,rdx
mul    QWORD [const_38]
add    r9,rax
mov    rax,r10
mov    r10,0x0
adc    r10,rdx
mul    QWORD [const_38]
add    r10,rax
mov    rax,r11
mov    r11,0x0
adc    r11,rdx
mul    QWORD [const_38]
add    r11,rax
mov    rax,0x0
adc    rax,rdx
add    r13,r8
adc    r14,r9
adc    r15,r10
adc    rbx,r11
mov    rdx,0x0
adc    rax,rdx
imul   r8,rax,0x26
add    r13,r8
adc    r14,rdx
adc    r15,rdx
adc    rbx,rdx
adc    rdx,rdx
imul   rdx,rdx,0x26
add    r13,rdx
mov    QWORD [rsp+0x38],r13
mov    QWORD [rsp+0x40],r14
mov    QWORD [rsp+0x48],r15
mov    QWORD [rsp+0x50],rbx
mov    r8,0x0
mov    r9,0x0
mov    r10,0x0
mov    r11,0x0
mov    r12,QWORD [rsi+0x40]
mov    rax,QWORD [rcx+0x40]
mul    r12
mov    r13,rax
mov    r14,rdx
mov    rax,QWORD [rcx+0x48]
mul    r12
add    r14,rax
mov    r15,0x0
adc    r15,rdx
mov    rax,QWORD [rcx+0x50]
mul    r12
add    r15,rax
mov    rbx,0x0
adc    rbx,rdx
mov    rax,QWORD [rcx+0x58]
mul    r12
add    rbx,rax
adc    r8,rdx
mov    r12,QWORD [rsi+0x48]
mov    rax,QWORD [rcx+0x40]
mul    r12
add    r14,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x48]
mul    r12
add    r15,rax
adc    rdx,0x0
add    r15,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x50]
mul    r12
add    rbx,rax
adc    rdx,0x0
add    rbx,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x58]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
adc    r9,rdx
mov    r12,QWORD [rsi+0x50]
mov    rax,QWORD [rcx+0x40]
mul    r12
add    r15,rax
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x48]
mul    r12
add    rbx,rax
adc    rdx,0x0
add    rbx,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x50]
mul    r12
add    r8,rax
adc    rdx,0x0
add    r8,rbp
mov    rbp,0x0
adc    rbp,rdx
mov    rax,QWORD [rcx+0x58]
mul    r12
add    r9,rax
adc    rdx,0x0
add    r9,rbp
adc    r10,rdx
mov    rsi,QWORD [rsi+0x58]
mov    rax,QWORD [rcx+0x40]
mul    rsi
add    rbx,rax
mov    r12,0x0
adc    r12,rdx
mov    rax,QWORD [rcx+0x48]
mul    rsi
add    r8,rax
adc    rdx,0x0
add    r8,r12
mov    r12,0x0
adc    r12,rdx
mov    rax,QWORD [rcx+0x50]
mul    rsi
add    r9,rax
adc    rdx,0x0
add    r9,r12
mov    r12,0x0
adc    r12,rdx
mov    rax,QWORD [rcx+0x58]
mul    rsi
add    r10,rax
adc    rdx,0x0
add    r10,r12
adc    r11,rdx
mov    rax,r8
mul    QWORD [const_38]
mov    rsi,rax
mov    rax,r9
mov    rcx,rdx
mul    QWORD [const_38]
add    rcx,rax
mov    rax,r10
mov    r8,0x0
adc    r8,rdx
mul    QWORD [const_38]
add    r8,rax
mov    rax,r11
mov    r9,0x0
adc    r9,rdx
mul    QWORD [const_38]
add    r9,rax
mov    rax,0x0
adc    rax,rdx
add    r13,rsi
adc    r14,rcx
adc    r15,r8
adc    rbx,r9
mov    rsi,0x0
adc    rax,rsi
imul   rdx,rax,0x26
add    r13,rdx
adc    r14,rsi
adc    r15,rsi
adc    rbx,rsi
adc    rsi,rsi
imul   rsi,rsi,0x26
add    r13,rsi
add    r13,r13
adc    r14,r14
adc    r15,r15
adc    rbx,rbx
mov    rsi,0x0
mov    rdx,0x26
cmovae rdx,rsi
add    r13,rdx
adc    r14,rsi
adc    r15,rsi
adc    rbx,rsi
cmovb  rsi,rdx
add    r13,rsi
mov    rsi,r13
mov    rdx,r14
mov    rcx,r15
mov    r8,rbx
add    rsi,QWORD [rsp+0x38]
adc    rdx,QWORD [rsp+0x40]
adc    rcx,QWORD [rsp+0x48]
adc    r8,QWORD [rsp+0x50]
mov    r9,0x0
mov    rax,0x26
cmovae rax,r9
add    rsi,rax
adc    rdx,r9
adc    rcx,r9
adc    r8,r9
cmovb  r9,rax
add    rsi,r9
sub    r13,QWORD [rsp+0x38]
sbb    r14,QWORD [rsp+0x40]
sbb    r15,QWORD [rsp+0x48]
sbb    rbx,QWORD [rsp+0x50]
mov    r9,0x0
mov    rax,0x26
cmovae rax,r9
sub    r13,rax
sbb    r14,r9
sbb    r15,r9
sbb    rbx,r9
cmovb  r9,rax
sub    r13,r9
mov    QWORD [rdi+0x20],rsi
mov    QWORD [rdi+0x28],rdx
mov    QWORD [rdi+0x30],rcx
mov    QWORD [rdi+0x38],r8
mov    QWORD [rdi+0x60],r13
mov    QWORD [rdi+0x68],r14
mov    QWORD [rdi+0x70],r15
mov    QWORD [rdi+0x78],rbx
mov    r11,QWORD [rsp]
mov    r12,QWORD [rsp+0x8]
mov    r13,QWORD [rsp+0x10]
mov    r14,QWORD [rsp+0x18]
mov    r15,QWORD [rsp+0x20]
mov    rbx,QWORD [rsp+0x28]
mov    rbp,QWORD [rsp+0x30]
add    rsp,r11
mov    rax,rdi
mov    rdx,rsi
ret

sV_ge64_p2_dbl:
mov    r11,rsp
and    r11,0x1f
add    r11,0xc0
sub    rsp,r11
mov    QWORD [rsp],r11
mov    QWORD [rsp+0x8],r12
mov    QWORD [rsp+0x10],r13
mov    QWORD [rsp+0x18],r14
mov    QWORD [rsp+0x20],r15
mov    QWORD [rsp+0x28],rbx
mov    QWORD [rsp+0x30],rbp
mov    rcx,0x0
mov    rax,QWORD [rsi+0x8]
mul    QWORD [rsi]
mov    r8,rax
mov    r9,rdx
mov    rax,QWORD [rsi+0x10]
mul    QWORD [rsi+0x8]
mov    r10,rax
mov    r11,rdx
mov    rax,QWORD [rsi+0x18]
mul    QWORD [rsi+0x10]
mov    r12,rax
mov    r13,rdx
mov    rax,QWORD [rsi+0x10]
mul    QWORD [rsi]
add    r9,rax
adc    r10,rdx
adc    r11,0x0
mov    rax,QWORD [rsi+0x18]
mul    QWORD [rsi+0x8]
add    r11,rax
adc    r12,rdx
adc    r13,0x0
mov    rax,QWORD [rsi+0x18]
mul    QWORD [rsi]
add    r10,rax
adc    r11,rdx
adc    r12,0x0
adc    r13,0x0
adc    rcx,0x0
add    r8,r8
adc    r9,r9
adc    r10,r10
adc    r11,r11
adc    r12,r12
adc    r13,r13
adc    rcx,rcx
mov    rax,QWORD [rsi]
mul    QWORD [rsi]
mov    r14,rax
mov    r15,rdx
mov    rax,QWORD [rsi+0x8]
mul    QWORD [rsi+0x8]
mov    rbx,rax
mov    rbp,rdx
mov    rax,QWORD [rsi+0x10]
mul    QWORD [rsi+0x10]
add    r8,r15
adc    r9,rbx
adc    r10,rbp
adc    r11,rax
adc    r12,rdx
adc    r13,0x0
adc    rcx,0x0
mov    rax,QWORD [rsi+0x18]
mul    QWORD [rsi+0x18]
add    r13,rax
adc    rcx,rdx
mov    rax,r11
mul    QWORD [const_38]
mov    r11,rax
mov    rax,r12
mov    r12,rdx
mul    QWORD [const_38]
add    r12,rax
mov    rax,r13
mov    r13,0x0
adc    r13,rdx
mul    QWORD [const_38]
add    r13,rax
mov    rax,rcx
mov    rcx,0x0
adc    rcx,rdx
mul    QWORD [const_38]
add    rcx,rax
mov    rax,0x0
adc    rax,rdx
add    r14,r11
adc    r8,r12
adc    r9,r13
adc    r10,rcx
mov    rdx,0x0
adc    rax,rdx
imul   rcx,rax,0x26
add    r14,rcx
adc    r8,rdx
adc    r9,rdx
adc    r10,rdx
adc    rdx,rdx
imul   rdx,rdx,0x26
add    r14,rdx
mov    QWORD [rsp+0x38],r14
mov    QWORD [rsp+0x40],r8
mov    QWORD [rsp+0x48],r9
mov    QWORD [rsp+0x50],r10
mov    rcx,0x0
mov    rax,QWORD [rsi+0x28]
mul    QWORD [rsi+0x20]
mov    r8,rax
mov    r9,rdx
mov    rax,QWORD [rsi+0x30]
mul    QWORD [rsi+0x28]
mov    r10,rax
mov    r11,rdx
mov    rax,QWORD [rsi+0x38]
mul    QWORD [rsi+0x30]
mov    r12,rax
mov    r13,rdx
mov    rax,QWORD [rsi+0x30]
mul    QWORD [rsi+0x20]
add    r9,rax
adc    r10,rdx
adc    r11,0x0
mov    rax,QWORD [rsi+0x38]
mul    QWORD [rsi+0x28]
add    r11,rax
adc    r12,rdx
adc    r13,0x0
mov    rax,QWORD [rsi+0x38]
mul    QWORD [rsi+0x20]
add    r10,rax
adc    r11,rdx
adc    r12,0x0
adc    r13,0x0
adc    rcx,0x0
add    r8,r8
adc    r9,r9
adc    r10,r10
adc    r11,r11
adc    r12,r12
adc    r13,r13
adc    rcx,rcx
mov    rax,QWORD [rsi+0x20]
mul    QWORD [rsi+0x20]
mov    r14,rax
mov    r15,rdx
mov    rax,QWORD [rsi+0x28]
mul    QWORD [rsi+0x28]
mov    rbx,rax
mov    rbp,rdx
mov    rax,QWORD [rsi+0x30]
mul    QWORD [rsi+0x30]
add    r8,r15
adc    r9,rbx
adc    r10,rbp
adc    r11,rax
adc    r12,rdx
adc    r13,0x0
adc    rcx,0x0
mov    rax,QWORD [rsi+0x38]
mul    QWORD [rsi+0x38]
add    r13,rax
adc    rcx,rdx
mov    rax,r11
mul    QWORD [const_38]
mov    r11,rax
mov    rax,r12
mov    r12,rdx
mul    QWORD [const_38]
add    r12,rax
mov    rax,r13
mov    r13,0x0
adc    r13,rdx
mul    QWORD [const_38]
add    r13,rax
mov    rax,rcx
mov    rcx,0x0
adc    rcx,rdx
mul    QWORD [const_38]
add    rcx,rax
mov    rax,0x0
adc    rax,rdx
add    r14,r11
adc    r8,r12
adc    r9,r13
adc    r10,rcx
mov    rdx,0x0
adc    rax,rdx
imul   rcx,rax,0x26
add    r14,rcx
adc    r8,rdx
adc    r9,rdx
adc    r10,rdx
adc    rdx,rdx
imul   rdx,rdx,0x26
add    r14,rdx
mov    QWORD [rsp+0x58],r14
mov    QWORD [rsp+0x60],r8
mov    QWORD [rsp+0x68],r9
mov    QWORD [rsp+0x70],r10
mov    rcx,0x0
mov    rax,QWORD [rsi+0x48]
mul    QWORD [rsi+0x40]
mov    r8,rax
mov    r9,rdx
mov    rax,QWORD [rsi+0x50]
mul    QWORD [rsi+0x48]
mov    r10,rax
mov    r11,rdx
mov    rax,QWORD [rsi+0x58]
mul    QWORD [rsi+0x50]
mov    r12,rax
mov    r13,rdx
mov    rax,QWORD [rsi+0x50]
mul    QWORD [rsi+0x40]
add    r9,rax
adc    r10,rdx
adc    r11,0x0
mov    rax,QWORD [rsi+0x58]
mul    QWORD [rsi+0x48]
add    r11,rax
adc    r12,rdx
adc    r13,0x0
mov    rax,QWORD [rsi+0x58]
mul    QWORD [rsi+0x40]
add    r10,rax
adc    r11,rdx
adc    r12,0x0
adc    r13,0x0
adc    rcx,0x0
add    r8,r8
adc    r9,r9
adc    r10,r10
adc    r11,r11
adc    r12,r12
adc    r13,r13
adc    rcx,rcx
mov    rax,QWORD [rsi+0x40]
mul    QWORD [rsi+0x40]
mov    r14,rax
mov    r15,rdx
mov    rax,QWORD [rsi+0x48]
mul    QWORD [rsi+0x48]
mov    rbx,rax
mov    rbp,rdx
mov    rax,QWORD [rsi+0x50]
mul    QWORD [rsi+0x50]
add    r8,r15
adc    r9,rbx
adc    r10,rbp
adc    r11,rax
adc    r12,rdx
adc    r13,0x0
adc    rcx,0x0
mov    rax,QWORD [rsi+0x58]
mul    QWORD [rsi+0x58]
add    r13,rax
adc    rcx,rdx
mov    rax,r11
mul    QWORD [const_38]
mov    r11,rax
mov    rax,r12
mov    r12,rdx
mul    QWORD [const_38]
add    r12,rax
mov    rax,r13
mov    r13,0x0
adc    r13,rdx
mul    QWORD [const_38]
add    r13,rax
mov    rax,rcx
mov    rcx,0x0
adc    rcx,rdx
mul    QWORD [const_38]
add    rcx,rax
mov    rax,0x0
adc    rax,rdx
add    r14,r11
adc    r8,r12
adc    r9,r13
adc    r10,rcx
mov    rdx,0x0
adc    rax,rdx
imul   rcx,rax,0x26
add    r14,rcx
adc    r8,rdx
adc    r9,rdx
adc    r10,rdx
adc    rdx,rdx
imul   rdx,rdx,0x26
add    r14,rdx
add    r14,r14
adc    r8,r8
adc    r9,r9
adc    r10,r10
mov    rdx,0x0
mov    rcx,0x26
cmovae rcx,rdx
add    r14,rcx
adc    r8,rdx
adc    r9,rdx
adc    r10,rdx
cmovb  rdx,rcx
add    r14,rdx
mov    QWORD [rsp+0x78],r14
mov    QWORD [rsp+0x80],r8
mov    QWORD [rsp+0x88],r9
mov    QWORD [rsp+0x90],r10
mov    rdx,0x0
mov    rcx,0x0
mov    r8,0x0
mov    r9,0x0
sub    rdx,QWORD [rsp+0x38]
sbb    rcx,QWORD [rsp+0x40]
sbb    r8,QWORD [rsp+0x48]
sbb    r9,QWORD [rsp+0x50]
mov    rax,0x0
mov    r10,0x26
cmovae r10,rax
sub    rdx,r10
sbb    rcx,rax
sbb    r8,rax
sbb    r9,rax
cmovb  rax,r10
sub    rdx,rax
mov    QWORD [rsp+0x38],rdx
mov    QWORD [rsp+0x40],rcx
mov    QWORD [rsp+0x48],r8
mov    QWORD [rsp+0x50],r9
mov    rax,0x0
mov    r10,0x0
mov    r11,0x0
mov    r12,0x0
sub    rax,QWORD [rsp+0x58]
sbb    r10,QWORD [rsp+0x60]
sbb    r11,QWORD [rsp+0x68]
sbb    r12,QWORD [rsp+0x70]
mov    r13,0x0
mov    r14,0x26
cmovae r14,r13
sub    rax,r14
sbb    r10,r13
sbb    r11,r13
sbb    r12,r13
cmovb  r13,r14
sub    rax,r13
mov    QWORD [rsp+0x98],rax
mov    QWORD [rsp+0xa0],r10
mov    QWORD [rsp+0xa8],r11
mov    QWORD [rsp+0xb0],r12
mov    rax,rdx
mov    r10,rcx
mov    r11,r8
mov    r12,r9
add    rax,QWORD [rsp+0x58]
adc    r10,QWORD [rsp+0x60]
adc    r11,QWORD [rsp+0x68]
adc    r12,QWORD [rsp+0x70]
mov    r13,0x0
mov    r14,0x26
cmovae r14,r13
add    rax,r14
adc    r10,r13
adc    r11,r13
adc    r12,r13
cmovb  r13,r14
add    rax,r13
mov    QWORD [rdi+0x20],rax
mov    QWORD [rdi+0x28],r10
mov    QWORD [rdi+0x30],r11
mov    QWORD [rdi+0x38],r12
sub    rdx,QWORD [rsp+0x58]
sbb    rcx,QWORD [rsp+0x60]
sbb    r8,QWORD [rsp+0x68]
sbb    r9,QWORD [rsp+0x70]
mov    r13,0x0
mov    r14,0x26
cmovae r14,r13
sub    rdx,r14
sbb    rcx,r13
sbb    r8,r13
sbb    r9,r13
cmovb  r13,r14
sub    rdx,r13
mov    QWORD [rdi+0x40],rdx
mov    QWORD [rdi+0x48],rcx
mov    QWORD [rdi+0x50],r8
mov    QWORD [rdi+0x58],r9
sub    rax,QWORD [rsp+0x78]
sbb    r10,QWORD [rsp+0x80]
sbb    r11,QWORD [rsp+0x88]
sbb    r12,QWORD [rsp+0x90]
mov    rdx,0x0
mov    rcx,0x26
cmovae rcx,rdx
sub    rax,rcx
sbb    r10,rdx
sbb    r11,rdx
sbb    r12,rdx
cmovb  rdx,rcx
sub    rax,rdx
mov    QWORD [rdi+0x60],rax
mov    QWORD [rdi+0x68],r10
mov    QWORD [rdi+0x70],r11
mov    QWORD [rdi+0x78],r12
mov    rdx,QWORD [rsi]
mov    rcx,QWORD [rsi+0x8]
mov    r8,QWORD [rsi+0x10]
mov    r9,QWORD [rsi+0x18]
add    rdx,QWORD [rsi+0x20]
adc    rcx,QWORD [rsi+0x28]
adc    r8,QWORD [rsi+0x30]
adc    r9,QWORD [rsi+0x38]
mov    rsi,0x0
mov    rax,0x26
cmovae rax,rsi
add    rdx,rax
adc    rcx,rsi
adc    r8,rsi
adc    r9,rsi
cmovb  rsi,rax
add    rdx,rsi
mov    QWORD [rsp+0x58],rdx
mov    QWORD [rsp+0x60],rcx
mov    QWORD [rsp+0x68],r8
mov    QWORD [rsp+0x70],r9
mov    rsi,0x0
mov    rax,QWORD [rsp+0x60]
mul    QWORD [rsp+0x58]
mov    rcx,rax
mov    r8,rdx
mov    rax,QWORD [rsp+0x68]
mul    QWORD [rsp+0x60]
mov    r9,rax
mov    r10,rdx
mov    rax,QWORD [rsp+0x70]
mul    QWORD [rsp+0x68]
mov    r11,rax
mov    r12,rdx
mov    rax,QWORD [rsp+0x68]
mul    QWORD [rsp+0x58]
add    r8,rax
adc    r9,rdx
adc    r10,0x0
mov    rax,QWORD [rsp+0x70]
mul    QWORD [rsp+0x60]
add    r10,rax
adc    r11,rdx
adc    r12,0x0
mov    rax,QWORD [rsp+0x70]
mul    QWORD [rsp+0x58]
add    r9,rax
adc    r10,rdx
adc    r11,0x0
adc    r12,0x0
adc    rsi,0x0
add    rcx,rcx
adc    r8,r8
adc    r9,r9
adc    r10,r10
adc    r11,r11
adc    r12,r12
adc    rsi,rsi
mov    rax,QWORD [rsp+0x58]
mul    QWORD [rsp+0x58]
mov    r13,rax
mov    r14,rdx
mov    rax,QWORD [rsp+0x60]
mul    QWORD [rsp+0x60]
mov    r15,rax
mov    rbx,rdx
mov    rax,QWORD [rsp+0x68]
mul    QWORD [rsp+0x68]
add    rcx,r14
adc    r8,r15
adc    r9,rbx
adc    r10,rax
adc    r11,rdx
adc    r12,0x0
adc    rsi,0x0
mov    rax,QWORD [rsp+0x70]
mul    QWORD [rsp+0x70]
add    r12,rax
adc    rsi,rdx
mov    rax,r10
mul    QWORD [const_38]
mov    r10,rax
mov    rax,r11
mov    r11,rdx
mul    QWORD [const_38]
add    r11,rax
mov    rax,r12
mov    r12,0x0
adc    r12,rdx
mul    QWORD [const_38]
add    r12,rax
mov    rax,rsi
mov    rsi,0x0
adc    rsi,rdx
mul    QWORD [const_38]
add    rsi,rax
mov    rax,0x0
adc    rax,rdx
add    r13,r10
adc    rcx,r11
adc    r8,r12
adc    r9,rsi
mov    rsi,0x0
adc    rax,rsi
imul   rdx,rax,0x26
add    r13,rdx
adc    rcx,rsi
adc    r8,rsi
adc    r9,rsi
adc    rsi,rsi
imul   rsi,rsi,0x26
add    r13,rsi
add    r13,QWORD [rsp+0x38]
adc    rcx,QWORD [rsp+0x40]
adc    r8,QWORD [rsp+0x48]
adc    r9,QWORD [rsp+0x50]
mov    rsi,0x0
mov    rdx,0x26
cmovae rdx,rsi
add    r13,rdx
adc    rcx,rsi
adc    r8,rsi
adc    r9,rsi
cmovb  rsi,rdx
add    r13,rsi
add    r13,QWORD [rsp+0x98]
adc    rcx,QWORD [rsp+0xa0]
adc    r8,QWORD [rsp+0xa8]
adc    r9,QWORD [rsp+0xb0]
mov    rsi,0x0
mov    rdx,0x26
cmovae rdx,rsi
add    r13,rdx
adc    rcx,rsi
adc    r8,rsi
adc    r9,rsi
cmovb  rsi,rdx
add    r13,rsi
mov    QWORD [rdi],r13
mov    QWORD [rdi+0x8],rcx
mov    QWORD [rdi+0x10],r8
mov    QWORD [rdi+0x18],r9
mov    r11,QWORD [rsp]
mov    r12,QWORD [rsp+0x8]
mov    r13,QWORD [rsp+0x10]
mov    r14,QWORD [rsp+0x18]
mov    r15,QWORD [rsp+0x20]
mov    rbx,QWORD [rsp+0x28]
mov    rbp,QWORD [rsp+0x30]
add    rsp,r11
mov    rax,rdi
mov    rdx,rsi
ret
