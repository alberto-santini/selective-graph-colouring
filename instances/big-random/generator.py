import sys
import random
print ("Script name: %s" % str(sys.argv[0]))
print ("node number: %s" % str(sys.argv[1]))
print ("density: %s" % str(sys.argv[2]))
print ("random seed: %s" % str(sys.argv[3]))
n=int(sys.argv[1])
p=n/2
dens=float(sys.argv[2])
ssd=int(sys.argv[3])

random.seed(ssd)
e=0
edge=[[],[]]
for i in range (n):
    for j in range (i+1,n):
        if random.random()<dens:
            edge[0].append(i)
            edge[1].append(j)
            e=e+1


outname = 'graph_'+str(n)+'_'+str(dens)+'_'+str(ssd)+'.txt'
f = open (outname,'w')
f.write('%d\n' % n)
f.write('%d\n' % e)
if (2*p == n):
    f.write('%d\n' % p)
else:
    f.write('%d\n' % (p+1))
for i in range (e):
    f.write('%d %d \n' % (edge[0][i],edge[1][i]))
for i in range (0,2*p,2):
    f.write('%d %d \n' % (i, i+1))
if (2*p < n):
    f.write('%d \n' % (n))
f.close()

print("expected edges %d --- actual edges %d \n" % (dens*n*(n-1)/2, e))

f = open ("graph_list.txt",'a+')
f.write('%s \n' % outname)
f.close()
