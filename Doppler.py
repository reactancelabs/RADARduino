import numpy as N
from numpy import *
import wave
import struct
import scipy as S
import scipy.io
import scipy.io.wavfile
import pylab as P

def dbv(input):
    return 20*N.log10(abs(input))

#def readwav(filename):
#    return scipy.io.wavfile.read(filename)

#creating a new readwav file to fix wav reading errors
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


def read_data_doppler(filename='Off of Newton Exit 17.wav', Tp=0.25, fc=2590E6 ):
  FS,Y=readwav(filename)

  #constants 
  c = 3E8
  
  #radar parameters
  #Tp = 0.250  # (s) pulse time
  n = int(Tp*FS)   # number of samples per pulse
  print ['Number of samples per pulse:', n]
  #fc = 2590E6 # (Hz) Center frequency (connected VCO Vtune to +5)
  
  #the input appears to be inverted
  s = -1*Y[0] #Y[:,1]
  del Y
  
  #create doppler vs. time plot data set here
  #for ii =1:round(size(s,1)/N)-1
  nsif=int(round(s.shape[0]/n)-1)
  print ['nsif:', nsif]
  sif=N.zeros([nsif,n])
  for ii in xrange(0,nsif):
      sif[ii,:] = s[ii*n:(ii+1)*n]
      #sif(ii,:) = s(1+(ii-1)*N:ii*N)
       
  #subtract the average DC term here
  sif = sif-s.mean()       
  
  zpad = 8*n/2
  
  #doppler vs. time plot:
  v=dbv(N.fft.ifft(sif, zpad,1))
  v=v[:,0:v.shape[1]/2]
  mmax = v.max()
  #calculate velocity
  delta_f = N.linspace(0, FS, v.shape[1]) #(Hz) 
  wavelen=c/fc
  velocity = delta_f*wavelen/2.
  #calculate time
  time = N.linspace(0,Tp*v.shape[0],v.shape[0]) #(sec)

  #plot
  P.pcolormesh(velocity[0:512], time, v[:,0:512]-mmax, edgecolors = 'None')
  P.plt.gca().invert_yaxis()
  P.matshow(v[:,0:1024], extent=(velocity[0], velocity[1023], time[-1], time[0]))
  P.colorbar()
  P.clim([-40,0])  

  P.xlabel('Velocity (m/sec)')
  P.ylabel('time (sec)')    
  P.show();
  return v,velocity,time

read_data_doppler('CarsPassingArtesia1.wav', 0.125, 5500E6)
#read_data_doppler('/Users/AlX/Dropbox/sar/Recordings/Aviation1.wav', 0.25, 2590E6 )
