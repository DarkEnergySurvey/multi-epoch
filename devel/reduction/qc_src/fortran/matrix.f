
	real*8 x(1000000), y(1000000), xmin, xmax, ymin, ymax
	integer ax,ay
	character* 40 filein

	n = 0

	do i=1,1000000
		read(*,*,end=10) x(i), y(i)
	enddo

 10	n = i - 1

	xmin = 360
	xmax = -360
	ymin = 360
	ymax = -360

	do i=1,n
		if (x(i).lt.xmin) then
			xmin = x(i)
		endif
		if (x(i).gt.xmax) then
			xmax = x(i)
		endif
		if (y(i).lt.ymin) then
			ymin = y(i)
		endif
		if (y(i).gt.ymax) then
			ymax = y(i)
		endif
	enddo

	ax = 150
	ay = 150

	call def_smooth_matrix(ax,ay,x,y,n,xmin,ymin,xmax,ymax)

	stop
	end


	subroutine def_smooth_matrix(ax,ay,x,y,n,xmin,ymin,xmax,ymax)
	real*8 x(1000000),y(1000000),xmin,ymin,xmax,ymax
	integer ax,ay,n
	integer C(ax+20,ay+20)
	real*8 xbin(ax),ybin(ay),Cs1(ax+10,ay+10),Cs(ax,ay)

	do i=1,ax+1
		xbin(i) = xmin + ((i-1)*(xmax-xmin))/ax
	enddo

	do i=1,ay+1
		ybin(i) = ymin + ((i-1)*(ymax-ymin))/ay
	enddo

	do i=1,ax+20
		do j=1,ay+20
			C(i,j) = 0
		enddo
	enddo

	do i=1,ax+10
		do j=1,ay+10
			Cs1(i,j) = 0
		enddo
	enddo

	do i=1,ax
		do j=1,ay
			Cs(i,j) = 0
		enddo
	enddo

	do i=1,ax
		do j=1,ay
			do k=1,n
				if ((x(k).ge.xbin(i)).and.(x(k).le.xbin(i+1))
     &				.and.(y(k).ge.ybin(j)).and.(y(k).le.ybin(j+1))) then
					C(i+10,j+10) = C(i+10,j+10) + 1
				endif
			enddo
		enddo
	enddo

	do i=1,ax
		do j=1,ay
			do k=-5,5
				do l=-5,5
					Cs1(i+5,j+5) = Cs1(i+5,j+5) + C(i+10+k,j+10+l)/121.0
				enddo
			enddo
		enddo
	enddo

	do i=1,ax
		do j=1,ay
			do k=-5,5
				do l=-5,5
					Cs(i,j) = Cs(i,j) + Cs1(i+5+k,j+5+l)/121.0
				enddo
			enddo
		enddo
	enddo


	do i=1,ax
		do j=1,ay
			if (j.eq.ay) then
				print 20, Cs(i,j)
 20				format(1x,f10.6)
			else
				print 21, Cs(i,j)
 21				format(1x,f10.6,$)
			endif
		enddo
	enddo

	return
	end




