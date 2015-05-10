import numpy as N
from numpy import *
import wave
import struct
import scipy as S
import pylab as P

def dbv(input):
    return 20*N.log10(abs(input))

#Functions to read-in a wave file
def everyOther (v, offset=0):
    return [v[i] for i in range(offset, len(v), 2)]

def readwav(filename):
    wav = wave.open (filename, "r")
    (nchannels, sampwidth, framerate, nframes, comptype, compname) = wav.getparams ()
    frames = wav.readframes (nframes * nchannels)
    out = struct.unpack_from ("%dh" % nframes * nchannels, frames)

    # Convert 2 channles to numpy arrays
    if nchannels == 2:
        left = array (list (everyOther (out, 0)))
        right = array (list  (everyOther (out, 1)))
    else:
        left = array (out)
        right = left
    Data=array([left, right])
    return [framerate, Data]

#Enter parameters here:
filename = 'Ranging_park_edited.wav'
Tp=14E-3
c=3E8
lfm=[5481E6,6136E6]


#%Process Range vs. Time Intensity (RTI) plot
FS,Y=readwav(filename)
n = int(Tp*FS)   # number of samples per pulse
print ['Number of samples per pulse:', n]
fstart = lfm[0] #(Hz) LFM start frequency
fstop = lfm[1] #(Hz) LFM stop frequency
BW = fstop-fstart #(Hz) transmit bandwidth
f = N.linspace(fstart, fstop, n/2) #instantaneous transmit frequency
rr = c/(2*BW)
max_range = rr*n/2
#the input appears to be inverted
trig = -1.*Y[1]
s = -1.*Y[0]
del Y
#parse the data here by triggering off rising edge of sync pulse
count = 0
thresh = 0
start = (trig > thresh)
#start2= N.zeros(start.shape)
time= []

nsif=int(round(s.shape[0]/n)-1)
sif=N.zeros([nsif,n])

for ii in xrange(100,(start.shape[0]-n)):
    if (start[ii] == True) & (start[ii-11:ii-1].mean() == 0):
        #start2[ii] = 1
        if (ii+n) < s.size and count<nsif:
            #print count
            sif[count,:] = s[ii:ii+n]
            time.append( ii*1./FS )
            count = count + 1
time=N.array(time[:-1])
#Remove no signal part.
sif=sif[:count,:]

##check to see if triggering works
"""P.plot(trig[0:5000]/15e3,'.b')
P.hold('on')#si
P.plot(start[0:5000],'.r')
P.hold('off')
P.grid('on')
P.show()"""
#Red dots mark the rising edge of the trigger signal, so it works.

#remove average.
sif= sif-N.tile(sif.mean(0), [sif.shape[0],1])
zpad = 8*n/2

#RTI plot
#P.figure()
v=dbv(N.fft.ifft(sif, zpad,1))
s=v[:,0:v.shape[1]/2]
print ['Shape of result: ', s.shape]
m=s.max()
#P.pcolormesh(N.linspace(0, max_range*800./s.shape[1], 800), time, s[:,0:800]-m, edgecolors = 'None')
#P.pcolormesh(N.linspace(0, max_range, zpad/2), time, s-m, edgecolors = 'None')
P.plt.gca().invert_yaxis()
P.matshow(s[0:time.shape[0]]-m)
P.colorbar()
P.ylabel('time (s)')
P.xlabel('range (m)')
P.title('RTI without clutter rejection')
P.clim([-80,0])

#2 pulse cancelor RTI plot
"""#P.figure()
sif2 = sif[1:sif.shape[0],:]-sif[0:sif.shape[0]-1,:]
v2 = N.fft.ifft(sif2,zpad,1) #S in matlab code
#for ii in xrange(1:v2.shape[0]):
#    v2[ii,:] = v2[ii,:]*R**(3./2.) #Optional: magnitude scale to range
s2 = dbv(v2[:,0:v2.shape[1]/2])
m = s2.max()
r = N.linspace(0,max_range,zpad/2)
P.pcolormesh(r, time, s2-m, edgecolors='None')
P.plt.gca().invert_yaxis()
P.clim([-80,0])
P.colorbar()
P.ylabel('time (s)')
P.xlabel('range (m)')
P.title('RTI with 2-pulse cancelor clutter rejection')"""
P.show()

#return [s-s.max(), s2-s2.max()]
